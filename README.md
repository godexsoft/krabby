# Krabby
A simple HTTP/WebSocket Lua scriptable webserver written in modern C++.

### Dependencies
Most of the dependencies are in git submodules under the `lib` directory.
To fetch Krabby with all its dependencies do this:
```
$ git clone https://github.com/godexsoft/krabby.git --recurse-submodules
```

You will also need `SQLite3` development package and `Lua5.1` or newer with its development package installed on your system. Krabby will use whatever `Lua` you have available (through `sol3`).

`CMake` is used for generation of your preferred build files. By default it will generate a `Makefile`.
You will need `CMake 3.15` or newer in order to succesfully configure Krabby.

### Compile
```
$ mkdir build
$ cd build
$ cmake -DENABLE_LOG=YES ..
$ make
```

### Usage:
See `examples/scripts` directory for Lua code.

Krabby accepts a data root directory as first (optional) argument. It will load all the `.lua` scripts it can find recursively.

```
$ krabby -h
$ krabby path/to/data/root
```

*NOTE:* Krabby will use current directory as data root if no path is specified.

### Docker

Docker image is available at https://hub.docker.com/r/godexsoft/krabby

```
$ docker run -d --rm -v /path/to/data/root:/data:rw -p 8080:8080 --name krabby godexsoft/krabby:0.0.1
```

### API

#### JSON
JSON manipulation is supported via the `json` type:
```
local data = json.new()      
local other = json.parse("{'some':'thing'}")

data:str("strParam", "some string")
data:int("intParam", 420)
data:dbl("doubleParam", 42.0)
data:obj("objOrArray", other)

-- note: same goes for str/int/dbl
local otherCopy = data:obj("objOrArray")
```

#### Templates
Output produced by Krabby can be generated using `Inja` templates by passing a filepath and a JSON object with data:
```
local output = template:render_file("templates/index.j2", data)    
print(output)
```

#### Timers
Timers can be set to fire like so:
```
local t = timer.new(
    function()
        print("timer fired")
    end)

t:once(3)
```

#### Key-Value Storage
Krabby creates and maintains an sqlite3 database which can be used as a high-efficient key-value storage.
You can save `ints`, `doubles`, `strings` and even JSON objects and arrays:

```
-- loads a JSON array with fallback to empty array
local records = storage:load("your_key", json.parse("[]"))

local data = json.new()
data:str("name", "Krabby")
data:int("age", 420)

-- add a new record and save it back to storage
records:push_back(data)
storage:save("your_key", records)
```

*Note:* The above code automatically serializes and deserializes the JSON object/array.

#### Basic HTTP
##### Writing Responses
Once you will learn about routes below you will know how to apply these functions, but for now here is what is available for writing data back to clients:
```
respond(who, 200, "text/html", "<html><body>plain html data</body></html>")
respond_html(who, 404, "<p>wrapped html 404 page</p>")
respond_text(who, 401, "not authorized text here")
respond_msg(who, "arbitrary text data for websocket")
```

##### Disconnect Handler
When a connection to Krabby is closed all functions that are setup with `Disconnect` will be called:
```
Disconnect( -- called on client disconnect
    function(who)
        -- react in some way if needed
    end )
```

##### Mountpoints
Mountpoints are used to expose static content at a given location on the filesystem. The paths are relative to `data root` specified at startup.

To expose `data/root/path/public` at `/public/*` on your server you can do this:
```
Mountpoint( "/public", "public" )
```

##### Routes
Routes can be used to setup dynamic endpoints. For example to make a RESTful API.
Supported routes are 
* Get
* Post
* Delete, Put and whatever else will be added later

The following example sets up a route at `/krabby/[3-16 characters long string without spaces]` that will be expecting `par1` and `par2` to be passed in the query string (i.e. `localhost:8080/krabby/hello?par1=first&par2=second`). If the parameters are not passed Krabby will automatically return an error page. All the parameters passed to this route will end up in `params` regardless of them being required or not.
The `matches` table will contain all the matches for the route path regular expression.
```
Get( "/krabby/(\\w{3,16})", {"par1", "par2"},
    function(who, req, matches, params)
        -- if this was called via 'localhost:8080/krabby/hello?par1=first&par2=second'
        -- matches[1] == "/krabby/hello"
        -- matches[2] == "hello"
        -- params["par1"] == "first"
        -- params["par2"] == "second"
    end )
```

*Note:* the route path does not have to be a `regular expression`

*Note:* if you don't require any parameters just pass `{}` for the list.

#### WebSockets
WebSocket communication is possible in Krabby. 
Here is an example of the server script for a WebSocket API:
```
api_connections = {} -- holds open connections over websockets

Get( "/api", {},
    function(who, req, matches, params)
        who:upgrade() -- upgrade to websocket connection
        api_connections[who.id] = true -- save connection for later
        respond_msg(who, "hello") -- send a message
    end )

Msg( -- called on websocket message
    function(who, msg)
        if not(api_connections[who.id]) then
            return false -- this is not our client but maybe another handler will handle it
        end
        
        respond_msg(who, "you said: "..msg.body)
        return true -- you need to return true to keep the connection alive
    end )
```

*Note:* Proper documentation will be written eventually.

#### Utils
There are a few utils included with Krabby
* generate_key(size) - generates a `size` long random alphanumeric key 
* hash_sha1(str) - computes sha1 for a string (raw bytes)
* hmac_sha1(str, secret) - computes hmac-sha1 for a string with a shared secret
* string_to_hex(bytes) - computes a hex string for input bytes
* escape_html(html) - escapes html string

*Note:* See `examples/scripts/utils.lua` for usage examples.

### Powered by:
* C++17
* https://github.com/hrissan/crablib
* https://github.com/GVNG/SQLCPPBridgeFramework
* https://github.com/ThePhD/sol2
* https://github.com/lua/lua (thru sol2)
* https://github.com/pantor/inja.git
* https://github.com/nlohmann/json (thru inja)
* https://github.com/jarro2783/cxxopts.git
* https://github.com/fmtlib/fmt  
