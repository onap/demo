#!/usr/bin/env python
'''
Simple dispatcher for the REST API.

Only intended for test purposes.

License
-------

Copyright(c) <2016>, AT&T Intellectual Property.  All other rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. All advertising materials mentioning features or use of this software
   must display the following acknowledgement:  This product includes
   software developed by the AT&T.
4. Neither the name of AT&T nor the names of its contributors may be used to
   endorse or promote products derived from this software without specific
   prior written permission.

THIS SOFTWARE IS PROVIDED BY AT&T INTELLECTUAL PROPERTY ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL AT&T INTELLECTUAL PROPERTY BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
'''

import logging
logger = logging.getLogger('collector.disp')

base_url = ''

template_404 = b'''POST {0}'''

def set_404_content(url):
    '''
    Called at initialization to set the base URL so that we can serve helpful
    diagnostics as part of the 404 response. 
    '''
    global base_url
    base_url = url
    return

def notfound_404(environ, start_response):
    '''
    Serve the 404 Not Found response.
    
    Provides diagnostics in the 404 response showing the hierarchy of valid
    REST resources.
    '''
    logger.warning('Unexpected URL/Method: {0} {1}'.format(
                                             environ['REQUEST_METHOD'].upper(),
                                             environ['PATH_INFO']))
    start_response('404 Not Found', [ ('Content-type', 'text/plain') ])
    return [template_404.format(base_url)]

class PathDispatcher:
    '''
    A dispatcher which can take HTTP requests in a WSGI environment and invoke
    appropriate methods for each request.
    '''
    def __init__(self):
        '''Constructor: initialize the pathmap to be empty.'''
        self.pathmap = { }

    def __call__(self, environ, start_response):
        '''
        The main callable that the WSGI app will invoke with each request.
        '''
        #----------------------------------------------------------------------
        # Extract the method and path from the environment.
        #----------------------------------------------------------------------
        method = environ['REQUEST_METHOD'].lower()
        path = environ['PATH_INFO']
        logger.info('Dispatcher called for: {0} {1}'.format(method, path))
        logger.debug('Dispatcher environment is: {0}'.format(environ))

        #----------------------------------------------------------------------
        # See if we have a handler for this path, and if so invoke it.
        # Otherwise, return a 404.
        #----------------------------------------------------------------------
        handler = self.pathmap.get((method, path), notfound_404)
        logger.debug('Dispatcher will use handler: {0}'.format(handler))
        return handler(environ, start_response)

    def register(self, method, path, function):
        '''
        Register a handler for a method/path, adding it to the pathmap.
        '''
        logger.debug('Registering for {0} at {1}'.format(method, path))
        print('Registering for {0} at {1}'.format(method, path))
        self.pathmap[method.lower(), path] = function
        return function
