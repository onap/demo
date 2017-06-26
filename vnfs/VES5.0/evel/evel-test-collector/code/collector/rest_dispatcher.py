#!/usr/bin/env python
'''
Simple dispatcher for the REST API.

Only intended for test purposes.

License
-------

 * ===================================================================
 * Copyright © 2017 AT&T Intellectual Property. All rights reserved.
 * ===================================================================
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.

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
