function get_url_parameter(sParam) 
{
    var sPageURL = decodeURIComponent(window.location.search.substring(1)),
        sURLVariables = sPageURL.split('&'),
        sParameterName,
        i;

    for (i = 0; i < sURLVariables.length; i++) {
        sParameterName = sURLVariables[i].split('=');

        if (sParameterName[0] === sParam) {
            return sParameterName[1] === undefined ? true : sParameterName[1];
        }
    }
}

var STATUS = { // 4.1
    OK : 1,
    FAIL : 0
};

var COMMANDS = {
    PING : "ping"
}

function is_ping_command(msgObj)
{
    return msgObj.command === COMMANDS.PING;    
}

function is_failed_command(msgObj)
{
    return msgObj.status === STATUS.FAIL;    
}

function is_succsess_command(msgObj)
{
    return msgObj.status === STATUS.OK;    
}

function parse_command_out(msg)
{
    var msg_length = msg.length;
    var pos = 0;
    var data = "";
    
    var id = 0;
    var status = STATUS.FAIL;
    var command = "";
    
    for (var i = 0; i < msg_length; i++)
    {
        var c = msg[i];
        if(c === ' '){
            if(pos === 0){
                id = parseInt(data, 10);
            }
            else if(pos === 1){
                if(data === "ok"){
                    status = STATUS.OK;
                }
                else if(data === "fail"){
                    status = STATUS.FAIL;
                }
                else{
                    break;
                }
            }
            else if(pos === 2){
                command = data;
                return {
                            id: id,
                            status: status,
                            command: command,
                            args : msg.substr(i, msg_length - i)
                        };
            }
            pos++;
            data = "";
        }
        else{
            data += c;
        }
    }
    
    return undefined;
}

function ping_server(id, name, socket)
{
    var msg = name + " " + id.toString() + " " + COMMANDS.PING;
    socket.emit('publish', msg);
}

