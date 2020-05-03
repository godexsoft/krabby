--
-- A simple example using kv-storage.
-- illustrates how one can use kv-store to save and load json objects and arrays
--

local __dbkey = "name_age_test"

Get ( "/db", {},     
    function(who, req, matches, params)
        local data = json.new()        
        data:obj("entries", storage:load(__dbkey, json.parse("[]")))

	    local output = template:render_file("templates/db/index.j2", data)
        respond(who, 200, "text/html", output)
    end )

Post ( "/db", {'name', 'age'},     
    function(who, req, matches, params)        
        local name = params["name"]
        if #name < 1 then name = "Krabby" end        
        local age = tonumber(params["age"]) or 420

        local records = storage:load(__dbkey, json.parse("[]"))    
        local data = json.new()
        data:str("name", name)
        data:int("age", age)

        records:push_back(data)
        storage:save(__dbkey, records)

        local output = template:render_file("templates/db/redirect.j2", json:new())
        respond(who, 200, "text/html", output)
    end )
    