-- 
-- JSON API request example
--

-- renders the index page
-- uses parameter 'name' or defaults to 'Krabby' for greeting via template
Get( "/apireq", {},
    function(who, req, matches, params)        

        client = ClientGet("https://c19downloads.azureedge.net/downloads/json/coronavirus-deaths_latest.json",
            function(resp)
                local data = json.parse(resp.body)
                local meta = data:obj("metadata")
                local today = string.sub(meta:str("lastUpdatedAt"), 1, 10) -- get the date portion out

                data:str("today", today)
                local output = template:render_file("templates/http_request/index.j2", data)
                respond(who, 200, "text/html", output)
            end, 
            function(err) 
                respond_html(who, 500, "Could not query api: "..err)        
            end )
        who:start_long_poll(
            function()
                client:cancel()
            end)
    end )

