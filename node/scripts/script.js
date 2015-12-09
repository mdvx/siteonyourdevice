// global constant
const CHANNEL_IN = 'COMMANDS_IN';
const CHANNEL_OUT = 'COMMANDS_OUT';
const CHANNEL_CLIENTS_STATE = 'CLIENTS_STATE';
const NODE_PORT = 3000;

//get url field
function get_url_parameter_url(url, sParam) 
{
    var sPageURL = decodeURIComponent(url),
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

function get_url_parameter(sParam) 
{
    return get_url_parameter_url(window.location.search.substring(1));
}

var STATUS = { 
    OK : 1,
    FAIL : 0
};

var CONNECTED_STATUS = { 
    CONNECTED : 1,
    DISCONNECTED : 0
};

var COMMANDS = {
    PING : "ping", //+
    INFO : "plz_system_info", //+
    SHUTDOWN : "plz_disconnect" //+
}

// state parse
function parse_state_msg(msg)
{
    var msg_length = msg.length;
    var pos = 0;
    var msg_host_state = msg.split(" ");
    if(msg_host_state.length === 2){
        return {host : msg_host_state[0], status : msg_host_state[1] == "connected" ? CONNECTED_STATUS.CONNECTED : CONNECTED_STATUS.DISCONNECTED }
    }
    
    return undefined;
}

// is_commands
function is_ping_command(msgObj)
{
    return msgObj.command === COMMANDS.PING;    
}
function is_info_command(msgObj)
{
    return msgObj.command === COMMANDS.INFO;    
}
function is_shutdown_command(msgObj)
{
    return msgObj.command === COMMANDS.SHUTDOWN;    
}
// status off command
function is_failed_command(msgObj)
{
    return msgObj.status === STATUS.FAIL;    
}

function is_succsess_command(msgObj)
{
    return msgObj.status === STATUS.OK;    
}
// parse string to msgObj

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

// server functions
function ping_server(id, name, socket)
{
    if(name === undefined){
        return;
    }
    
    var msg = name + " " + id + " " + COMMANDS.PING;
    socket.emit('publish', msg);
}

function shutdown_server(id, name, socket)
{
    if(name === undefined){
        return;
    }
    
    var msg = name + " " + id + " " + COMMANDS.SHUTDOWN;
    socket.emit('publish', msg);
}

function server_info(id, name, socket)
{
    if(name === undefined){
        return;
    }
    
    var msg = name + " " + id + " " + COMMANDS.INFO;
    socket.emit('publish', msg);
}

//// server_details constant
const SERVER_DEFAULT_LABEL = "Unknown";
const SERVER_CPU_DEFAULT_LABEL = "Unknown"
const SERVER_OS_DEFAULT_LABEL = "Unknown"
const SERVER_RAM_DEFAULT_LABEL = "Unknown"
var SERVER_STATUS = { 
    ONLINE : "online",
    OFFLINE : "offline"
};
