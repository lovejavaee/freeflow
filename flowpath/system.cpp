#include <exception>
#include <unordered_map>
#include <cstdarg>

#include "system.hpp"
#include "port_table.hpp"
#include "application.hpp"


namespace fp
{


Module_table     module_table;          // Flowpath module table.
Dataplane_table  dataplane_table;       // Flowpath data plane table.
Port_table       port_table;            // Flowpath port table.
Thread_pool      thread_pool(0, true);  // Flowpath thread pool.


// Creates a new port, adds it to the master port table, and
// returns a pointer to the new port.
Port*
create_port(Port::Type port_type, std::string const& args)
{
  // Create the port of given type with args in the master port table.
  Port* p = port_table.alloc(port_type, args);

  // Return a pointer to it.
  return p;
}


// Deletes the given port from the system port table.
void
delete_port(Port::Id id)
{
  if (port_table.find(id))
    port_table.dealloc(id);
}


// Creates a new data plane and returns a pointer to it. If the
// name already exists it throws an exception.
Dataplane*
create_dataplane(std::string const& name, std::string const& app)
{
  // Check if a dataplane with this name already exists.
  if (dataplane_table.find(name) != dataplane_table.end())
    throw std::string("Data plane name already exists");

  // Allocate the new data plane in the master data plane table.
  dataplane_table.insert({name, new Dataplane(name, app)});

  return dataplane_table.at(name);
}


// Deletes the given data plane from the system data plane table.
void
delete_dataplane(std::string const& name)
{
  auto dp = dataplane_table.find(name);
  if (dp != dataplane_table.end())
    dataplane_table.erase(dp);
  else
    throw std::string("Data plane name not in use");
}


// Loads the application at the given path. If it exists, throws a message.
// If the application does not exist, it creates the module and adds it to
// the module table.
void
load_application(std::string const& path)
{
  // Check if library from this path has already been loaded.
  if (module_table.find(path) != module_table.end())
    throw std::string("Application at '" + path + "' has already been loaded");

  // Register the path with the Application_library object.
  module_table.insert({path, new Application(path)});
}


// Unloads the given application. If it does not exist, throws a message.
void
unload_application(std::string const& path)
{
  auto app = module_table.find(path);
  // Check if library from this path has already been loaded.
  if (app != module_table.end())
    throw std::string("Application at '" + path + "' is not loaded.");

  // Remove the application from the module table.
  module_table.erase(app);
}


} // end namespace fp


//////////////////////////////////////////////////////////////////////////
//                    External Runtime System Calls                     //
//////////////////////////////////////////////////////////////////////////
//
// These are the set of system calls that an application can expect
// to be able to call at runtime.


extern "C"
{


// -------------------------------------------------------------------------- //
// Control instructions


// Send the packet through the drop port.
void
fp_drop(fp::Context* cxt)
{
  // Cache the drop port so the lookup doesn't happen every time.
  static fp::Port* drop = fp::port_table.drop_port();
  drop->send(cxt);
}


void
fp_flood(fp::Context* cxt)
{
  // Cache the drop port so the lookup doesn't happen every time.
  static fp::Port* flood = fp::port_table.flood_port();
  flood->send(cxt);
}


// Apply the given action to the context.
void
fp_apply(fp::Context* cxt, fp::Action a)
{
  cxt->apply_action(a);
}


// Write the given action to the context's action list.
void
fp_write(fp::Context* cxt, fp::Action a)
{
  cxt->write_action(a);
}


// Clear the context's action list.
void
fp_clear(fp::Context* cxt)
{
  cxt->clear_actions();
}


// Dispatches the given context to the given table, if it exists.
// Accepts a variadic list of fields needed to construct a key to
// match against the table.
void
fp_goto_table(fp::Context* cxt, fp::Table* tbl, int n, ...)
{
  va_list args;
  va_start(args, n);
  fp::Key key = fp_gather(cxt, tbl->key_size(), n, args);
  va_end(args);
  // find the flow
  fp::Flow const& flow = tbl->find(key);
  // execute the flow function
  flow.instr_(tbl, cxt);
}


// -------------------------------------------------------------------------- //
// Port and table operations

// Returns the port matching the given name.
fp::Port*
fp_get_port(char const* name)
{
  return fp::port_table.find(name);
}


// Outputs the contexts packet on the port.
void
fp_output_port(fp::Context* cxt, fp::Port* p)
{
  p->send(cxt);
}


// Copies the values within 'n' fields into a byte buffer
// and constructs a key from it.
fp::Key
fp_gather(fp::Context* cxt, int key_width, int n, va_list args)
{
  // FIXME: We're using a fixed size key of 128 bytes right now
  // apparently. This should probably be dynamic.
  fp::Byte buf[fp::key_size];
  // Iterate through the fields given in args and copy their
  // values into a byte buffer.
  int i = 0;
  int j = 0;
  while (i < n) {
    int f = va_arg(args, int);
    // Lookup the field in the context.
    fp::Binding b = cxt->get_field_binding(f);
    fp::Byte* p = cxt->get_field(b.offset);
    // Copy the field into the buffer.
    std::copy(p, p + b.length, &buf[j]);
    j += b.length;
  }

  return fp::Key(buf, key_width);
}


// Creates a new table in the given data plane with the given size,
// key width, and table type.
fp::Table*
fp_create_table(fp::Dataplane* dp, int id, int size, int key_width, fp::Table::Type type)
{
  fp::Table* tbl = nullptr;
  switch (type)
  {
    case fp::Table::Type::EXACT:
      // Make a new hash table.
      tbl = new fp::Hash_table(id, size, key_width);
      dp->tables().push_back(tbl);
      break;
    case fp::Table::Type::PREFIX:
      // Make a new prefix match table.
      break;
    case fp::Table::Type::WILDCARD:
      // Make a new wildcard match table.
      break;
    default:
    throw std::string("Unknown table type given");
  }
  return tbl;
}


// Creates a new flow rule from the given key and function pointer
// and adds it to the given table.
void
fp_add_flow(fp::Table* tbl, void* key, void* fn)
{
  // get the length of the table's expected key
  int key_size = tbl->key_size();
  // cast the key to Byte*
  fp::Byte* buf = reinterpret_cast<fp::Byte*>(key);
  // construct a key object
  fp::Key k(buf, key_size);
  // cast the flow into a flow instruction
  fp::Flow_instructions instr = reinterpret_cast<fp::Flow_instructions>(fn);
  fp::Flow flow(0, fp::Flow_counters(), instr, fp::Flow_timeouts(), 0, 0);
  tbl->insert(k, flow);
}


// Adds the miss case for the table.
void
fp_add_miss(fp::Table* tbl, void* fn)
{
  // cast the flow into a flow instruction
  fp::Flow_instructions instr = reinterpret_cast<fp::Flow_instructions>(fn);
  fp::Flow flow(0, fp::Flow_counters(), instr, fp::Flow_timeouts(), 0, 0);
  tbl->insert_miss(flow);
}


// Removes the given key from the given table, if it exists.
void
fp_del_flow(fp::Table* tbl, void* key)
{
  // get the length of the table's expected key
  int key_size = tbl->key_size();
  // cast the key to Byte*
  fp::Byte* buf = reinterpret_cast<fp::Byte*>(key);
  // construct a key object
  fp::Key k(buf, key_size);
  // delete the key
  tbl->erase(k);
}


// -------------------------------------------------------------------------- //
// Header and field bindings



// Advances the current header offset by 'n' bytes.
void
fp_advance_header(fp::Context* cxt, std::uint16_t n)
{
  cxt->advance(n);
}


// Binds the current header offset to given identifier.
void
fp_bind_header(fp::Context* cxt, int id)
{
  cxt->bind_header(id);
}


// Binds a given field index to a section in the packet contexts raw
// packet data. Using the current cxt offset, relative field offset, and field
// length we can grab exactly what we need.
//
// Returns the pointer to the byte at that specific location
fp::Byte*
fp_bind_field(fp::Context* cxt, int id, std::uint16_t off, std::uint16_t len)
{
  // Get field requires an absolute offset which is the context's current offset
  // plus the relative offset passed to this function.
  int abs_off = cxt->offset() + off;
  // We bind fields using their absolute offset since this is the only way we
  // can recover the absolute offset when we need to look up the binding later.
  //
  // FIXME: There needs to be a way to store the relative offset instead of the
  // absolute offset.
  cxt->bind_field(id, abs_off, len);
  return cxt->get_field(abs_off);
}



} // extern "C"
