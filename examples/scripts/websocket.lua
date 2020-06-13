--
-- A simple WebSocket communication example
-- see templates/ws/index.j2 for JavaScript client code
--

Get ( "/ws", {},
    function(who, req, matches, params)
        local output = template:render_file("templates/ws/index.j2", json.new())
        respond(who, 200, "text/html", output)
    end )

-- websocket api example
Get( "/ws/api", {},
    function(who, req, matches, params)        
        who:upgrade(
            function(msg)
                print("web socket msg")
                if msg.body ~= "bye" then
                    respond_msg(who, "you said: "..msg.body)
                    return true -- you need to return true to keep the connection alive
                end
                -- will close connection here because implicit false is returned
            end,
            function()
                print("web socket closed")
            end ) -- upgrade to websocket connection
        respond_msg(who, "hello")

        -- setup a timer to spit some message out
        local t = timer.new(
            function()
                respond_msg(who, "timer") -- client side will send "bye" back to this message
            end)
            
        t:once(3)
    end )
