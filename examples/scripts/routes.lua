-- renders the index page
-- uses parameter 'name' or defaults to 'Krabby' for greeting via template
Get( "/", {},
    function(who, req, matches, params)
        local data = json.new()        
        data:str("name", params["name"] or "Krabby")

	local output = template:render_file("templates/index.j2", data)
        html_response(who, 200, output)
    end )

-- usage: navigate to serverurl:port/krabby/anytext?par1=value1&par2=value2
-- result: displays the parsed regex matches and parameters
Get( "/krabby/(\\w{3,16})", {"par1", "par2"},
    function(who, req, matches, params)
        local output = "matches:<ul>"
        
        for key,value in pairs(matches) do 
            output = output.."<li>"..value.."</li>"
        end
        
        output = output.."</ul>fields:<ul>"
        for key,value in pairs(params) do 
            output = output.."<li>"..key.."="..value.."</li>"
        end

        -- if you don't respond, socket will stay open for a while and timeout eventually
        html_response(who, 200, "<h3>Krabby is happy</h3>"..output.."</ul>")
    end)
