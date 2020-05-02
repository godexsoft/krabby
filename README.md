# Krabby

### Compile
```
    mkdir build
    cd build
    cmake -DENABLE_LOG=YES ..
    make
```

### Usage:
See `examples/scripts` directory for Lua code.

Krabby accepts a data root directory as first (optional) argument. It will load all the `.lua` scripts it can find recursively.

```
    $ krabby -h
    $ krabby path/to/data/root
```

*NOTE:* Krabby will use current directory as data root if no path is specified.

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

### Powered by:
* https://github.com/hrissan/crablib
* https://github.com/GVNG/SQLCPPBridgeFramework
* https://github.com/ThePhD/sol2
* https://github.com/pantor/inja.git
* https://github.com/nlohmann/json
* https://github.com/jarro2783/cxxopts.git
* https://github.com/fmtlib/fmt  
