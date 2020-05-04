-- 
-- This is an example of a RESTful API
--
local __restful_users_dbkey = "restful_users_list" -- an array of keys

function api_fail(who, status, msg) 
    local wrapper = json.new()
    wrapper:bool("success", false)
    wrapper:str("message", msg)
    return respond(who, status, "application/json", wrapper:dump())
end

Get( "/restful", {},
    function(who, req, matches, params)
        local output = template:render_file("templates/restful/index.j2", json.new())
        respond(who, 200, "text/html", output)
    end )

-- GET user list (only the keys)
Get( "/restful/users", {},
    function(who, req, matches, params)
        local data = storage:load(__restful_users_dbkey, string_vector.new())        
        
        local wrapper = json:new()
        wrapper:bool("success", true)
        wrapper:vec("users", data)

        respond(who, 200, "application/json", wrapper:dump())
    end )

-- POST to add a new user 
Post( "/restful/users", {},
    function(who, req, matches, params)
        local data = storage:load(__restful_users_dbkey, string_vector.new())
        local payload = json.parse(req.body)
        local name = payload:str("name")
        local pass = payload:str("password")

        -- sanity check incoming data
        if not(name) or not(pass) then
            return api_fail(who, 400, "required fields: name, password")
        end        

        -- check if user with this name already exists
        local empty = json.new()
        for k,v in pairs(data) do                        
            local user = storage:load(v, empty)
            if user:str("name") == name then
                return api_fail(who, 418, "username already taken")                
            end
        end
        
        -- no user with name exists so lets create one
        local new_user = json.new()
        new_user:str("name", name)
        new_user:str("password", string_to_hex(hash_sha1(pass)))

        local new_key = generate_key(16)        
        storage:save(new_key, new_user)
        
        data:push_back(new_key)
        storage:save(__restful_users_dbkey, data)
        
        local wrapper = json.new()
        wrapper:bool("success", true)
        wrapper:str("key", new_key)        
        respond(who, 200, "application/json", wrapper:dump())
    end )
    
-- GET a single user by its key
Get( "/restful/users/(\\w{16})", {}, 
    function(who, req, matches, params)        
        local key = matches[2]
        
        local user = storage:load(key, json.new())
        if user.empty then
            return api_fail(who, 404, "user not found")            
        end
        
        local wrapper = json.new()
        wrapper:bool("success", true)
        wrapper:obj("user", user)        
        respond(who, 200, "application/json", wrapper:dump())
    end )

-- DELETE a user by key
Delete( "/restful/users/(\\w{16})", {}, 
    function(who, req, matches, params)        
        local key = matches[2]
        
        local user = storage:load(key, json.new())
        if user.empty then
            local wrapper = json.new()
            return api_fail(who, 404, "user not found")
        end
        
        storage:remove(key) -- remove the record itself
        storage:remove(__restful_users_dbkey, key) -- remove from the list of keys
        
        local wrapper = json.new()
        wrapper:bool("success", true)
        respond(who, 200, "application/json", wrapper:dump())
    end )

-- PUT a user by key (update password)
Put( "/restful/users/(\\w{16})", {}, 
    function(who, req, matches, params)        
        local key = matches[2]

        local payload = json.parse(req.body)
        local pass = payload:str("password")

        -- sanity check incoming data
        if not(pass) then
            return api_fail(who, 400, "required fields: password")
        end        

        local user = storage:load(key, json.new())
        if user.empty then
            local wrapper = json.new()
            return api_fail(who, 404, "user not found")
        end
        
        user:str("password", string_to_hex(hash_sha1(pass)))
        storage:save(key, user)
        
        local wrapper = json.new()
        wrapper:bool("success", true)
        respond(who, 200, "application/json", wrapper:dump())
    end )