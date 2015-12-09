const CHANNEL_IN = 'COMMANDS_IN';
const CHANNEL_OUT = 'COMMANDS_OUT';
const CHANNEL_CLIENTS_STATE = 'CLIENTS_STATE';
const NODE_PORT = 3000;

var app = require('http').createServer(handler)
var io = require('socket.io').listen(app)
var fs = require('fs')
var redis = require("redis")

function handler (req, res) {
    fs.readFile('server_details.html',
    function (err, data) {
        if (err) {
            res.writeHead(500);
            return res.end('Error loading server_details.html, folder' + __dirname);
        }
        
        var user_id = get_url_parameter(req.url, 'id');
        var user_cookie_hash = get_url_parameter(req.url, 'hash');
        console.log(user_id);
        console.log(req.user_cookie_hash);

        console.log('req cookie');
        var req_cookie = req.headers.cookie;
        var req_user_id = get_url_parameter_url(req_cookie, 'id');
        var req_user_cookie_hash = get_url_parameter_url(req_cookie, 'hash');
        console.log(req_user_id);
        console.log(req_user_cookie_hash);
        
        res.writeHead(200);
        res.end(data);
    });
}

app.listen(NODE_PORT);

var redis_sub = redis.createClient();
var redis_pub = redis.createClient();

redis_sub.on("error", function (err) {
    console.log("redis_sub error" + err);
});

redis_pub.on("error", function (err) {
    console.log("redis_pub error " + err);
});

io.sockets.on('connection', function (socket) {
    socket.on('subscribe', function (data) {
        socket.join(data.channel);
    });

    socket.on('publish', function (msg) {
        redis_pub.publish(CHANNEL_IN, msg);
    });

});

redis_sub.on('ready', function() {
    redis_sub.subscribe(CHANNEL_OUT, CHANNEL_CLIENTS_STATE);
});

redis_sub.on("message", function(channel, message){
    var resp = {'text': message, 'channel':channel}
    io.sockets.in(channel).emit('message', resp);
});

