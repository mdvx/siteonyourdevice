var app = require('http').createServer(handler)
var io = require('socket.io').listen(app)
var fs = require('fs')
var redis = require("redis")

app.listen(3000);

redis_sub = redis.createClient();
redis_pub = redis.createClient();

redis_sub.on("error", function (err) {
    console.log("error event - " + redis_sub.host + ":" + redis_sub.port + " - " + err);
});

redis_pub.on("error", function (err) {
    console.log("error event2 - " + redis_pub.host + ":" + redis_pub.port + " - " + err);
});

function handler (req, res) {
    fs.readFile('server_details.html',
    function (err, data) {
        if (err) {
            res.writeHead(500);
            return res.end('Error loading server_details.html' + __dirname);
        }
        res.writeHead(200);
        res.end(data);
    });
}

io.sockets.on('connection', function (socket) {
    socket.on('subscribe', function (data) {
        socket.join(data.channel);
    });

    socket.on('publish', function (msg) {
        redis_pub.publish('COMMANDS_IN', msg);
    });

});

redis_sub.on('ready', function() {
    redis_sub.subscribe('COMMANDS_OUT');
});

redis_sub.on("message", function(channel, message){
    var resp = {'text': message, 'channel':channel}
    io.sockets.in(channel).emit('message', resp);
});

