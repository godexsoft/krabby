api_connections = {} -- holds open connections over websockets

-- websocket api example
Get( "/api", {},
    function(who, req, matches, params)        
        who:upgrade() -- upgrade to websocket connection
        api_connections[who.id] = true -- save connection for later

        msg_response(who, "hello")

        -- setup a timer to spit some message out
        local t = timer.new(
            function()
                msg_response(who, "krabby timer message")
            end)
            
        t:once(3)
    end )

Msg( -- called on websocket message
    function(who, msg)
        if not(api_connections[who.id]) then
            return false -- this is not our client but maybe another handler will handle it
        end
        
        msg_response(who, "you said: "..msg.body)
        return true -- you need to return true to keep the connection alive
    end )

Disconnect( -- called on client disconnect
    function(who)
        -- check if it's our connection first
        if api_connections[who.id] then            
            api_connections[who.id] = nil -- remove client from known connections
        end
    end )