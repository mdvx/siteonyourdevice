About SiteOnYourDevice
===============
[![Travis Build Status](https://travis-ci.org/fastogt/siteonyourdevice.svg?branch=master)](https://travis-ci.org/fastogt/siteonyourdevice)

Site on your device - solution which connects your device with internet.
Remote control any computer/mobile/device over the internet.

**How to use:**<br/>
1) You register new account.<br/>
2) Register domain which you want.<br/>
3) Download our client(lightweight and fast Web Server).<br/>
4) Install and start client, specify path which web content you want share with internet.<br/>
5) Tell your friends about your own web site.

**Our client(Web Server) has next features:**
- Http 1.1 protocol.
- Http 2 protocol.
- WebSockets support.
- IPV6 connections.
- Proxy to external http server.
- Basic & digest authentication for secure access.
- Directory index.
- Cross-platform support.
- Open source.

Compare with standard internet domains:

**Advantages:**<br/>
  1) Free domain address which proxies over siteonyourdevice.com site.<br/>
  2) Free hosting because you use your device.<br/>
  3) Work on all devices.<br/>
  4) Remote control access to device.<br/>
  5) Direct access to web server if server is placed on visible network.

**Disadvantages:**<br/>
  1) It is not direct domain (proxies over siteonyourdevice.com).<br/>
  2) Young solution.<br/>
  3) Support only clear html/js page

**Handlers**<br/>
  You can add handlers url in your siteonyourdevice.ini file next options:
  ```
  [http_handlers_utls]
  shutdown = system::shutdown
  reboot = system::reboot
  logout = system::logout
  ```
  examples of html page you can see here http://siteonyourdevice.com/templates/
 
**WebSockets**<br/>
  Port 8060 on proxy.siteonyourdevice.com allocated for proxy websockets connection.<br/>
  Simple add next line on your server config file. 
  ```
  [http_server_sockets]
  websocket=ws://localhost:8088/echo
  ```
  on html page write next:
  ```
  ws://proxy.siteonyourdevice.com:8060/<your site>:8088/echo
  ```
  When your websocket connected to ```http://proxy.siteonyourdevice.com:8060/<your site>:8088/echo```,
  we redirect this call to your device into websocketserver link ws://localhost:8088/echo.

**External server proxing**<br/>
  Add next line in server config:
  ```
  [http_server]
  external_host = siteonyourdevice.com:80
  server_type = 1
  ```
  
**Private site**<br/>
  If you want that your site will be available only for you add next:
  ```
  [http_server]
  private_site = 1
  ```
  After that all connection to domain available only after passing phase of authentication.


**Style**<br/>
cpplint --linelength=100 --filter=-build/header_guard
