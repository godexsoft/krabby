# Krabby
A simple HTTP/WebSocket Lua scriptable webserver written in modern C++.

### Dependencies
Most of the dependencies are in git submodules under the `lib` directory.
To fetch Krabby with all its dependencies do this:
```
$ git clone https://github.com/godexsoft/krabby.git --recurse-submodules
```

You will also need `SQLite3` development package and `Lua5.1` or newer with its development package installed on your system. Krabby will use whatever `Lua` you have available (through `sol3`).

For example, on Ubuntu:
```
$ sudo apt-get install lua5.1-dev
$ sudo apt-get install libsqlite3-dev
```

`CMake` is used for generation of your preferred build files. By default it will generate a `Makefile`.
You will need `CMake 3.15` or newer in order to succesfully configure Krabby.

### Compile
```
$ mkdir build
$ cd build
$ cmake -DENABLE_LOG=ON -DCRAB_TLS=ON ..
$ make
```

*NOTE:*: CRAB_TLS option is only needed if you wish to include SSL support for the client request. For it to work you need to have OpenSSL installed.

*NOTE:*: You can specify the root of your OpenSSL installation (for MacOSX with brew for example): -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl

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
$ docker run -d --rm -v /path/to/data/root:/data:rw -p 8080:8080 --name krabby godexsoft/krabby:0.0.3
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
data:bool("boolParam", true)
data:obj("objOrArray", other)

-- note: same goes for str, int, dbl and bool
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

t:once(1.23)
```

#### Key-Value Storage
Krabby creates and maintains an sqlite3 database which can be used as a high-efficient key-value storage.
It operates on `strings`, `arrays of strings` (string_vector) or `JSON objects`.

Example using `JSON` directly:
```
-- loads a JSON array with fallback to empty array
local records = storage:load("your_key", json.array())

local data = json.new()
data:str("name", "Krabby")
data:int("age", 420)

-- add a new record and save it back to storage
records:push_back(data)
storage:save("your_key", records)
```

Example using `string_vector`:
```
local data = storage:load("your key", string_vector.new())
local new_user = json.new()
new_user:str("name", name)

local new_key = generate_key(16)        
storage:save(new_key, new_user) -- save JSON object under key
        
data:push_back(new_key)
storage:save("list key", data)
```

Removing data:
```
-- `user_key` is a string key for an existing user
storage:remove(user_key) -- remove the JSON object itself
storage:remove("user list key", user_key) -- remove it from a string_vector list of users
```

#### Live Reload/Recompile
Krabby is maintaining two contexts, one `master` and one `staging` context. You can reload the staging context at any time by using `Reload()` anywhere in your Lua code. This will rediscover and recompile all lua scripts under `data root path` and if everything compiles fine it will swap the current `master` context with the newly created `staging`:

```
local output = Reload()
if #output > 0 then
    print("compilation errors", output)
end
```

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
Mount( "/public", "public" )
```

##### Routes
Routes can be used to setup dynamic endpoints. For example to make a RESTful API.
Supported routes are 
* Get
* Post
* Delete 
* Put 
* Patch

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

#### Client Requests
You can also fetch data from a remote server using `ClientGet`. This works for both `http` and `https`:

```
handle = ClientGet("https://yourhost.com/api.json", 
    function(resp) -- called on response                
        local data = json.parse(resp.body)        
        local output = template:render_file("templates/mytemplate.j2", data)
        respond(who, 200, "text/html", output)
    end, 
    function(err) -- called on error
        respond_html(who, 500, "Could not query api: "..err)        
    end )

-- to cancel the request:
handle:cancel()
```

*Note:* The request will automatically get destroyed with no callbacks if it is not saved in a variable/table.

*Note:* See `examples/scripts/http_request.lua` for a more detailed example.

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
