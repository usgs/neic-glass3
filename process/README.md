# process
neic-glass3 associator (glasscore) hosting library

The neic-glass3 process library is a library that hosts an instance of the
glasscore association library. Process also manages communication, including
directing configuration updates, input data, and output messages.

It is named process not associator to avoid confusion between it and the actual
association library, glasscore.

This library has no unit tests due to the need to set up glasscore, which
would effectively be identical to glass-app.
