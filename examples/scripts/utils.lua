-- demonstrates a few utils available in Krabby
Get( "/util", {},
    function(who, req, matches, params)
        local data = json.new()      
        local secret = "krabby"  
        local str = "Krabby loves you <3"
        data:str("str", str)
        data:str("hmac", secret)

        -- generate a random 32 characters long key with perfect distribution
        local key = generate_key(32)
        data:str("key", key)
        
        -- compute a sha1 hash for the string
        local sha1 = hash_sha1(str)
        data:str("sha1", sha1)

        -- hexify sha1
        local sha1hex = string_to_hex(sha1)
        data:str("sha1hex", sha1hex)

        -- compute a hmac sha1 with a shared secret
        local hmacsha1 = hmac_sha1(str, secret)
        data:str("hmacsha1", hmacsha1)
        
        local html = "<b>Krabby</b>"
        local escaped = escape_html(html)
        data:str("escaped", escaped)
                
	    local output = template:render_file("templates/utils/index.j2", data)
        respond(who, 200, "text/html", output)
    end )