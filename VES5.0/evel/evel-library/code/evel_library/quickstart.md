# Quick Start Guide {#quickstart}

# Introduction {#qs_intro}

This Quick-Start section describes how to:

  *  Install and compile the supplied library code
  *  Integrate an existing project to use the EVEL library
  
# Installation {#qs_install}

The library is supplied as a source-code compressed-tar file. It is 
straightforward to install and build to integrate with an existing or new
development project.

## Unpack the Source Code {#qs_unpack}

The file should unpacked into your development environment:
```
$ mkdir evel
$ cd evel
$ tar zxvf evel-library-package.tgz
```
### Satisfy Dependencies {#qs_depend}

Note that all commands in this section are based on CentOS package management
tools and you may need to substitute the appropriate tools/packages for your
distribution, for example `apt-get` for Ubuntu.

Ensure that GCC development tools are available.

```
$ sudo yum install gcc
```
Additionally, the library has a dependency on the cURL library, so you'll need 
the development tools for libCurl installed. (At runtime, only the runtime 
library is required, of course.)

```
$ sudo yum install libcurl-devel
```
If you wish to make the project documentation, then Doxygen and Graphviz are
required. (Again, this is only in the development environment, not the runtime 
environment!)

```
$ sudo yum install doxygen graphviz
```

Note that some distributions have quite old versions of Doxygen by default and 
it may be necessary to install a later version to use all the features.

If you want to build PDFs from the LaTeX you will need a texlive install.

```
$ sudo yum install texlive
```

### Test Build {#qs_build}
Make sure that the library makes cleanly:

```
$ cd bldjobs
$ make
Making dependency file evel_unit.d for evel_unit.c
Making dependency file evel_test_control.d for evel_test_control.c
Making dependency file evel_demo.d for evel_demo.c
Making dependency file jsmn.d for jsmn.c
Making dependency file evel_logging.d for evel_logging.c
Making dependency file evel_event_mgr.d for evel_event_mgr.c
Making dependency file evel_internal_event.d for evel_internal_event.c
Making dependency file evel_throttle.d for evel_throttle.c
Making dependency file evel_syslog.d for evel_syslog.c
Making dependency file evel_strings.d for evel_strings.c
Making dependency file evel_state_change.d for evel_state_change.c
Making dependency file evel_scaling_measurement.d for evel_scaling_measurement.c
Making dependency file evel_signaling.d for evel_signaling.c
Making dependency file evel_service.d for evel_service.c
Making dependency file evel_reporting_measurement.d for evel_reporting_measurement.c
Making dependency file evel_json_buffer.d for evel_json_buffer.c
Making dependency file evel_other.d for evel_other.c
Making dependency file evel_option.d for evel_option.c
Making dependency file evel_mobile_flow.d for evel_mobile_flow.c
Making dependency file evel_fault.d for evel_fault.c
Making dependency file evel_event.d for evel_event.c
Making dependency file double_list.d for double_list.c
Making dependency file ring_buffer.d for ring_buffer.c
Making dependency file metadata.d for metadata.c
Making dependency file evel.d for evel.c
Making evel.o from evel.c
Making metadata.o from metadata.c
Making ring_buffer.o from ring_buffer.c
Making double_list.o from double_list.c
Making evel_event.o from evel_event.c
Making evel_fault.o from evel_fault.c
Making evel_mobile_flow.o from evel_mobile_flow.c
Making evel_option.o from evel_option.c
Making evel_other.o from evel_other.c
Making evel_json_buffer.o from evel_json_buffer.c
Making evel_reporting_measurement.o from evel_reporting_measurement.c
Making evel_service.o from evel_service.c
Making evel_signaling.o from evel_signaling.c
Making evel_scaling_measurement.o from evel_scaling_measurement.c
Making evel_state_change.o from evel_state_change.c
Making evel_strings.o from evel_strings.c
Making evel_syslog.o from evel_syslog.c
Making evel_throttle.o from evel_throttle.c
Making evel_internal_event.o from evel_internal_event.c
Making evel_event_mgr.o from evel_event_mgr.c
Making evel_logging.o from evel_logging.c
Making jsmn.o from jsmn.c
Linking API Shared Library
Linking API Static Library
Making evel_demo.o from evel_demo.c
Making evel_test_control.o from evel_test_control.c
Linking EVEL demo
Making EVEL training
$
``` 
You should now be able to run the demo CLI application.  Since it will want to
dynamically link to the library that you've just made, you will need to set 
your `LD_LIBRARY_PATH` appropriately first. Make sure that you specify
your actual directory paths correctly in the following:

```
$ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/centos/evel/libs/x86_64
$ ../output/x86_64/evel_demo
evel_demo [--help]
          --fqdn <domain>
          --port <port_number>
          [--path <path>]
          [--topic <topic>]
          [--username <username>]
          [--password <password>]
          [--https]
          [--cycles <cycles>]
          [--nothrott]

Demonstrate use of the ECOMP Vendor Event Listener API.

  -h         Display this usage message.
  --help

  -f         The FQDN or IP address to the RESTful API.
  --fqdn

  -n         The port number the RESTful API.
  --port

  -p         The optional path prefix to the RESTful API.
  --path

  -t         The optional topic part of the RESTful API.
  --topic

  -u         The optional username for basic authentication of requests.
  --username

  -w         The optional password for basic authentication of requests.
  --password

  -s         Use HTTPS rather than HTTP for the transport.
  --https

  -c         Loop <cycles> times round the main loop.  Default = 1.
  --cycles

  -v         Generate much chattier logs.
  --verbose

  -x         Exclude throttling commands from demonstration.
  --nothrott

$
```
Assuming that all worked as expected, you are ready to start integrating with 
your application. It probably makes sense to make the LD_LIBRARY_PATH change
above permanent by incorporating it into your `.bash_profile` file.

### Project Documentation {#qs_build_docs}

The source comes with its own documentation included. The documentation can be
built using the `docs` target in the Makefile. By default this builds HTML
and LaTeX documentation, the latter being used to prepare PDFs.

To make the documentation:
```
$ cd bldjobs
$ make docs
Cleaning docs...
Making Doxygen documentation
$ 
```

There is a make target that is intended to install the documentation on a
"team server" - it will need adaptation for your team's environment - see the 
`docs_install` target in the Makefile:

```
$ make docs_install
Cleaning docs...
Making Doxygen documentation
Copying docs to team web-server...
Enter passphrase for key '/data/home/.ssh/id_rsa':
annotated.html                           100% 8088     7.9KB/s   00:00    
arrowdown.png                            100%  246     0.2KB/s   00:00    
arrowright.png                           100%  229     0.2KB/s   00:00    
  ...
$
```

# Project Integration {#qs_integrate}

There are two key steps to the integration which have to be undertaken:

  * Initialization/Termination of the library.
  * Creation & posting of individual events.
  
Additionally, it may be necessary to consider changes to the EVEL library's
source code if assumptions made by the library are either not satisfied or 
inconvenient.  In particular:

  * If the project already uses libcurl then the global initialization of the
    library should be removed from the _EVEL Library_.
  * The _EVEL Library_ uses `syslog` for event logging. If the project uses a
    different event logging process, then EVEL's event logging macros should be
    rewritten appropriately.

These steps are considered in the [Normal Use](@ref qs_normal_use) and 
[EVEL Adaptation](@ref qs_adaptation) sections below.

## Normal Use         {#qs_normal_use}

The _EVEL Library_ should be integrated with your project at a per-process 
level: each process is an independent client of the ECOMP Vendor Event Listener
API.

### Initialization {#qs_initialize}

The _EVEL Library_ should be initialized before the process becomes 
multi-threaded. This constraint arises from the use of libcurl which imposes 
the constraint that initialization occurs before the system is multi-threaded.
This is described in more detail in the libcurl documentation for the
[curl_global_init](https://curl.haxx.se/libcurl/c/curl_global_init.html) 
function.

Initialization stores configuration of the Vendor Event Listener API's details,
such as the FQDN or IP address of the service, so the initializing process must
have either extracted this information from its configuration or have this 
information "hard-wired" into the application, so that it is available at the 
point the `evel_initialize()` function is called:

```C
  #include "evel.h"
  ...
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
```
Once initialization has occurred successfully, the application may raise events
and may also use the logging functions such as EVEL_INFO().

Initialization is entirely local (there is no interaction with the service) so
it is very unlikely to fail, unless the application environment is seriously 
degraded.

### Event Generation {#qs_generate}

Generating events is a two stage process:

  1.  Firstly, the _EVEL Library_ is called to allocate an event of the correct
      type. 
    * If this is successful, the caller is given a pointer to the event.
    * All mandatory fields on the event are provided to this factory function
      and are thereafter immutable.
    * The application may add any necessary optional fields to the event, using
      the pointer previously returned.
  2.  The event is sent to the JSON API using the evel_post_event() function.
    * At this point, the application relinquishes all responsibility for the 
      event:
      * It will be posted to the JSON API, if possible.
      * Whether or not the posting is successful, the memory used will be 
        freed.
        
In practice this looks like:

```C
  #include "evel.h"
  ...

  /***************************************************************************/
  /* Create a new Fault object, setting mandatory fields as we do so...      */
  /***************************************************************************/
  fault = evel_new_fault("My alarm condition",
                         "It broke very badly",
                         EVEL_PRIORITY_NORMAL,
                         EVEL_SEVERITY_MAJOR);
  if (fault != NULL)
  {
    /*************************************************************************/
    /* We have a Fault object - add some optional fields to it...            */
    /*************************************************************************/
    evel_fault_type_set(fault, "Bad things happen...");
    evel_fault_interface_set(fault, "My Interface Card");
    evel_fault_addl_info_add(fault, "name1", "value1");
    evel_fault_addl_info_add(fault, "name2", "value2");
    
    /*************************************************************************/
    /* Finally, post the Fault.  In practice this will only ever fail if     */
    /* local ring-buffer is full because of event overload.                  */
    /*************************************************************************/
    evel_rc = evel_post_event((EVENT_HEADER *)fault);
    if (evel_rc != EVEL_SUCCESS)
    {
      EVEL_ERROR("Post failed %d (%s)", evel_rc, evel_error_string());
    }
  }
  ...
```
### Event Types {#qs_event_types}

The _EVEL Library_ supports the following types of events:

  1.  Faults
  
      These represent the **fault** domain in the event schema.
      
  2.  Measurements
  
      These represent the **measurementsForVfScaling** domain in the event
      schema.
      
  3.  Reports
  
      This is an experimental type, designed to allow VNFs to report 
      application-level statistics unencumbered with platform measurements.
      The formal AT&T schema has been updated to include this experimental
      type as **measurementsForVfReporting**. 

  4.  Mobile Flow

      These represent the **mobileFlow** domain in the event schema.

  5.  Other

      These represent the **other** domain in the event schema.

  6.  Service Events

      These represent the **serviceEvents** domain in the event schema.

  7.  Signaling

      These represent the **signaling** domain in the event schema.

  8.  State Change

      These represent the **stateChange** domain in the event schema.

  9.  Syslog

      These represent the **syslog** domain in the event schema.

### Throttling {#qs_throttling}

The _EVEL library_ supports the following command types as defined in the JSON API:

  1.  commandType: throttlingSpecification

    This is handled internally by the EVEL library, which stores the provided
    throttling specification internally and applies it to all subsequent events.

  2. commandType: provideThrottlingState

    This is handled internally by the EVEL library, which returns the current
    throttling specification for each domain.

  3. commandType: measurementIntervalChange

    This is handled by the EVEL library, which makes the latest measurement
    interval available via the ::evel_get_measurement_interval function.
    The application is responsible for checking and adhering to the latest
    provided interval.

### Termination {#qs_termination}

Termination of the _EVEL Library_ is swift and brutal!  Events in the buffer
at the time are "dropped on the floor" rather than waiting for the buffer to 
deplete first.

```C
  #include "evel.h"
  ...
  
  /***************************************************************************/
  /* Shutdown the library.                                                   */
  /***************************************************************************/
  evel_terminate();
  
  ...
``` 

## EVEL Adaptation       {#qs_adaptation}

The _EVEL Library_ is relatively simple and should be easy to adapt into other
project environments.

### LibcURL Lifecycle

There are two circumstances where initialization of libcurl may be required:

  1.  If libcurl is used by the project already, and therefore already takes 
      responsibility of its initialization, then the libcurl initialization and 
      termination functions should be removed from evel_initialize() and 
      evel_terminate() respectively.
  2.  If the project is unable to satisfy the constraint that libcurl 
      initialization takes place in a single-threaded environment at the point
      that the _EVEL Library_ can be initialized (for example, if MT code is 
      necessary to read the configuration parameters required for 
      _EVEL Library_ initialization) then it may be necessary to extract the 
      libcurl functions and call them separately, earlier in the program's 
      operation.
      
### Event Logging

The _EVEL Library_ uses `syslog` for logging.  If this is inappropriate then
the log_debug() and log_initialize() functions should be rewritten accordingly.

**Note**: it would be a really bad idea to use the _EVEL Library_ itself for this 
logging function. 
[Turtles all the way down...](https://en.wikipedia.org/wiki/Turtles_all_the_way_down)
  
  