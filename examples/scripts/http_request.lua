-- 
-- JSON API request example
--

local cli_requests = {} -- map from who.id to an array of client requests

-- renders the index page
-- uses parameter 'name' or defaults to 'Krabby' for greeting via template
Get( "/apireq", {},
    function(who, req, matches, params)        
        requests = cli_requests[who.id] or {}        

        table.insert(requests, ClientGet("https://c19downloads.azureedge.net/downloads/json/coronavirus-deaths_latest.json", 
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
            end ))
        cli_requests[who.id] = requests        
    end )

Disconnect( -- called on client disconnect
    function(who)
        if cli_requests[who.id] then
            for k,v in pairs(cli_requests[who.id]) do
                v:cancel() -- cancel request if needed
            end
            cli_requests[who.id] = nil
        end
    end)