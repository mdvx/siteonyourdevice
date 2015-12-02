var app = require('http').createServer(handler)
var io = require('socket.io').listen(app)
var fs = require('fs')
var redis = require("redis")

app.listen(3000);

/** 
 * Our redis client which subscribes to channels for updates
 */
redisClient = redis.createClient();
redisClient2 = redis.createClient();

//look for connection errors and log
redisClient.on("error", function (err) {
    console.log("error event - " + redisClient.host + ":" + redisClient.port + " - " + err);
});

//look for connection errors and log
redisClient2.on("error", function (err) {
    console.log("error event2 - " + redisClient.host + ":" + redisClient.port + " - " + err);
});


/**
 * http handler, currently just sends server_details.html on new connection
 */
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

/**
 * socket io client, which listens for new websocket connection
 * and then handles various requests
 */
io.sockets.on('connection', function (socket) {
  //on subscription request joins specified room
  //later messages are broadcasted on the rooms
  socket.on('subscribe', function (data) {
    socket.join(data.channel);
  });

  socket.on('publish', function (msg) {
    console.log("publish:", msg);
    redisClient2.publish('COMMANDS_IN', msg);
  });

});

/**
 * subscribe to redis channel when client in ready
 */
redisClient.on('ready', function() {
  redisClient.subscribe('COMMANDS_OUT');
});

/**
 * wait for messages from redis channel, on message
 * send updates on the rooms named after channels. 
 * 
 * This sends updates to users. 
 */
redisClient.on("message", function(channel, message){
    var resp = {'text': message, 'channel':channel}
    io.sockets.in(channel).emit('message', resp);
});

