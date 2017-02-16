# EVEL Library Overview {#mainpage}

# Introduction

The ECOMP Vendor Event Listener ("EVEL") library encapsulates the use of
AT&T's JSON API to the collector function within the ECOMP infrastructure.

As such, it provides a reference implementation of the EVEL JSON API which
can either be directly as part of a project or can be used to inform the
independent implementation of an equivalent binding to the API in another
development environment.

This section provides an overview of the library and how it is integrated
into the target application.  If all you want is a set of instructions to
get you started, the @ref quickstart "Quick Start" section is for you.  If
you want a more in-depth understanding of the _EVEL Library_ then this section
provides an overview and then you can read the detailed API documentation for 
each function. The documentation for evel.h is a good starting point, since 
that defines the public API of the _EVEL Library_.

# Library Structure 

The API is designed to be used on multi-process platforms where each process
may be multi-threaded.  Each process using this library will create an
independent HTTP client (using libcURL).  Each process will have a single
thread running the HTTP client but that thread receives work on a
ring-buffer from however may threads are required to implement the function.

**Note**: libcurl imposes a constraint that it is initialized before
the process starts multi-threaded operation.

# Typical Usage

The library is designed to be very straightforward to use and lightweight to
integrate into projects. The only serious external dependency is on libcURL.

The supplied Makefile produces a single library **libevel.so** or
**libevel.a** which your application needs to be linked against.

Each process within the application which wants to generate events needs to
call ::evel_initialize at the start of day (observing the above warning
about not being MT safe at this stage.)   The initialization specifies the
details of where the API is located.  Management of configuration is the
responsibility of the client.

Once initialized, and now MT-safe, there are factory functions to produce
new events:
- Faults  - ::evel_new_fault
- Measurements - ::evel_new_measurement
- Report - ::evel_new_report
- State Change - ::evel_new_state_change
- Syslog - ::evel_new_syslog
- Other - ::evel_new_other
- Mobile Flow - ::evel_new_mobile_flow

There is also a factory function ::evel_new_mobile_gtp_flow_metrics to create
the parameter gtp_per_flow_metrics, which is then configured and passed to the
::evel_new_mobile_flow factory function.

The event structures are initialized with mandatory fields at the point of
creation and optional fields may be added thereafter.  Once set, values in
the structures are immutable.

Once the event is prepared, it may be posted, using ::evel_post_event,  at
which point the calling thread relinquishes all responsibility for the
event.  It will be freed once successfully or unsuccessfully posted to the
API.  If, for any reason, you change your mind and don't want to post a
created event, it must be destroyed with ::evel_free_event.

Finally, at the end of day, the library can be terminated cleanly by calling
::evel_terminate.

## Example Code

The following fragment illustrates the above usage:

```C

  if (evel_initialize(api_fqdn,
                      api_port,
                      api_path,
                      api_topic,
                      api_secure,
                      "Alice",
                      "This isn't very secure!",
                      EVEL_SOURCE_VIRTUAL_MACHINE,
                      "EVEL demo client",
                      verbose_mode))
  {
    fprintf(stderr, "Failed to initialize the EVEL library!!!");
    exit(-1);
  }

  ...

  fault = evel_new_fault("My alarm condition",
                         "It broke very badly",
                         EVEL_PRIORITY_NORMAL,
                         EVEL_SEVERITY_MAJOR);
  if (fault != NULL)
  {
    evel_fault_type_set(fault, "Bad things happen...");
    evel_fault_interface_set(fault, "My Interface Card");
    evel_fault_addl_info_add(fault, "name1", "value1");
    evel_fault_addl_info_add(fault, "name2", "value2");
    evel_rc = evel_post_event((EVENT_HEADER *)fault);
    if (evel_rc != EVEL_SUCCESS)
    {
      EVEL_ERROR("Post failed %d (%s)", evel_rc, evel_error_string());
    }
  }

```

The public API to the library is defined in evel.h.  The internal APIs
within library are defined in separate headers (<em>e.g.</em>
evel_internal.h), but these should not need to be included by the code
using the library.

# Example Application

A simple command-line application to generate events is provided as part of
the source package (the above code fragment is taken from that application).

The following illustrates its operation to a co-located "test-collector":
```
$ ./evel_demo --fqdn 127.0.0.1 --port 30000 --path vendor_event_listener --topic example_vnf --verbose
./evel_demo built Feb 26 2016 18:14:48
* About to connect() to 169.254.169.254 port 80 (#0)
*   Trying 169.254.169.254... * Timeout
* connect() timed out!
* Closing connection #0
* About to connect() to 127.0.0.1 port 30000 (#0)
*   Trying 127.0.0.1... * connected
* Connected to 127.0.0.1 (127.0.0.1) port 30000 (#0)
* Server auth using Basic with user 'Alice'
> POST /vendor_event_listener/eventListener/v1/example_vnf HTTP/1.1
Authorization: Basic QWxpY2U6VGhpcyBpc24ndCB2ZXJ5IHNlY3VyZSE=
User-Agent: libcurl-agent/1.0
Host: 127.0.0.1:30000
Accept: */*
Content-type: application/json
Content-Length: 510

* HTTP 1.0, assume close after body
< HTTP/1.0 204 No Content
< Date: Fri, 04 Mar 2016 15:37:22 GMT
< Server: WSGIServer/0.1 Python/2.6.6
< 
* Closing connection #0
* About to connect() to 127.0.0.1 port 30000 (#0)
*   Trying 127.0.0.1... * connected
* Connected to 127.0.0.1 (127.0.0.1) port 30000 (#0)
* Server auth using Basic with user 'Alice'
> POST /vendor_event_listener/eventListener/v1/example_vnf HTTP/1.1
Authorization: Basic QWxpY2U6VGhpcyBpc24ndCB2ZXJ5IHNlY3VyZSE=
User-Agent: libcurl-agent/1.0
Host: 127.0.0.1:30000
Accept: */*
Content-type: application/json
Content-Length: 865

* HTTP 1.0, assume close after body
< HTTP/1.0 204 No Content
< Date: Fri, 04 Mar 2016 15:37:22 GMT
< Server: WSGIServer/0.1 Python/2.6.6
< 
* Closing connection #0
* About to connect() to 127.0.0.1 port 30000 (#0)
*   Trying 127.0.0.1... * connected
* Connected to 127.0.0.1 (127.0.0.1) port 30000 (#0)
* Server auth using Basic with user 'Alice'
> POST /vendor_event_listener/eventListener/v1/example_vnf HTTP/1.1
Authorization: Basic QWxpY2U6VGhpcyBpc24ndCB2ZXJ5IHNlY3VyZSE=
User-Agent: libcurl-agent/1.0
Host: 127.0.0.1:30000
Accept: */*
Content-type: application/json
Content-Length: 2325

* HTTP 1.0, assume close after body
< HTTP/1.0 204 No Content
< Date: Fri, 04 Mar 2016 15:37:22 GMT
< Server: WSGIServer/0.1 Python/2.6.6
< 
* Closing connection #0
^C

Interrupted - quitting!
$
```

# Restrictions and Limitations

## Constraint Validation

The _EVEL Library_ has been designed to be production-safe code with the
emphasis at this stage being in correctness of operation rather than
raw performance.

The API tries to check as much information as possible to avoid misuse and
will **assert()** if constraints are not satisfied.  This is likely to lead
to the rapid discovery of coding errors by programmers, but does mean that
the application can fail abruptly if the library is misused in any way.

## Performance

The default Makefile avoids aggressive optimizations so that any core-files
are easy to interpret.  Production code should use greater optimization
levels.

As described above, the HTTP client is single threaded and will run all
transactions synchronously.  As transactions are serialized, a client that
generates a lot of events will be paced by the round-trip time.

It would be a straightforward enhancement to use the multi-thread API into
libcurl and use a pool of client threads to run transactions in parallel if
this ever became a bottleneck.

## Logging

The initialization of the library includes the log verbosity.  The verbose
operation makes the library very chatty so syslog may get rather clogged
with detailed diagnostics.  It is possible to configure syslog to put these
events into a separate file.  A trivial syslog.conf file would be:

```

# Log all user messages so debug information is captured.

user.*      /var/log/debug
```

If verbose logging is enabled, the cURL library will generate information 
about the HTTP operations on **stdout**. 

