#ifndef FP_APPLICATION_HPP
#define FP_APPLICATION_HPP


namespace fp
{

class Dataplane;
class Port;
class Context;


// The Library class represents a dynamically loaded application.
struct Library
{
  using Init_fn = int (*)(Dataplane*);
  using Proc_fn = int (*)(Context*);

  Library(char const*);
  ~Library();

  char const* path;
  void*       handle;

  Init_fn load;
  Init_fn unload;
  Init_fn start;
  Init_fn stop;
  Proc_fn proc;
};


// An application is a user-defined program that executes
// on a dataplane.
class Application
{
public:
  // State of the application
  enum State { INIT, READY, RUNNING, STOPPED };

  Application(char const* name)
    : lib_(name), state_(INIT)
  { }

  int load(Dataplane&);
  int unload(Dataplane&);
  int start(Dataplane&);
  int stop(Dataplane&);

  int process(Context&);

  // Returns the underlying library.
  Library const& library() const { return lib_; }
  Library&       library()       { return lib_; }

  // Returns the current application state.
  State state() const { return state_; }

  Library lib_;
  State   state_;
};


} // end namespace fp

#endif
