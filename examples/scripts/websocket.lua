--
-- A simple WebSocket communication example
-- see templates/ws/index.j2 for JavaScript client code
--

api_connections = {} -- holds open connections over websockets

Get ( "/ws", {}, 
    function(who, req, matches, params)
        local output = template:render_file("templates/ws/index.j2", json.new())
        respond(who, 200, "text/html", output)
    end )

-- websocket api example
Get( "/ws/api", {},
    function(who, req, matches, params)        
        who:upgrade() -- upgrade to websocket connection
        api_connections[who.id] = true -- save connection for later

        respond_msg(who, "hello")

        -- setup a timer to spit some message out
        local t = timer.new(
            function()
                respond_msg(who, "timer") -- client side will send "bye" back to this message
            end)
            
        t:once(3)
    end )

Msg( -- called on websocket message
    function(who, msg)
        if not(api_connections[who.id]) then
            return false -- this is not our client but maybe another handler will handle it
        end
        
        if msg.body ~= "bye" then
            respond_msg(who, "you said: "..msg.body)
            return true -- you need to return true to keep the connection alive
        end
        -- will close connection here because implicit false is returned
    end )

Disconnect( -- called on client disconnect
    function(who)
        -- check if it's our connection first
        if api_connections[who.id] then            
            api_connections[who.id] = nil -- remove client from known connections
        end
    end )