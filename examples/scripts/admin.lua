Get ( "/admin", {},     
    function(who, req, matches, params)
        local output = template:render_file("templates/admin/index.j2", json.new())
        respond(who, 200, "text/html", output)
    end )
    
Get ( "/admin/reload", {},     
    function(who, req, matches, params)
        local output = Reload()

        local wrapper = json.new()
        wrapper:bool("success", #output == 0)
        wrapper:str("message", output)
        respond(who, 200, "application/json", wrapper:dump())
    end )