# PowerDaemon
stupid program i (github copilot) made for myself basically it lets you make a http get requst to an endpoint and it does the thign on ur device based on the endpoint. default port is 8000 change the source code urself if u want a different one i aint doing allat

you need a file called allowed_ip.txt and u put a single line with the ip you want to allow requests from. idek if it works if u put 0.0.0.0 or something for all ips cause im only doing it from my phone anyway idc

and you need psshutdown.exe in the same directory as powerdaemon.exe. both are provided in releases. its psshutdown.exe from sysinternals

it puts a icon in the system tray with a exit button in context menu dont even know if it works

# Endpoints
/sleep /hibernate /shutdown /restart

# awesome example

GET http://10.0.0.1:8000/sleep
