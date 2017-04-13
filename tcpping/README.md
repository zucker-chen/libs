tcpping
=======

- I wrote this a few years ago, hope it will help someone who got the same need as I do before.
- This is a TCP verion ping by using RST/ACK mechanism in TCP protocol
-- It requires root privilege since it uses RAW SOCKET and create it's own TCP packet in user space
-- type "sudo ./tcpping -h" will give you what you need, e.g.:

> Usage:
> ./tcpping [options] <address>
> options:
> -c --count <count>             set send how many packets
> -s --source-port <port>        Source port (default is 54321)
> -p --port <port>               Destination port (default is 80)
> -w --window <size>             TCP window size (default is 32792)
> -m --mss <size>                TCP Maximum segment size (default is 16496)
> -t --timeout <seconds>         packet wait timeout (default is 5 seconds)
> -n                             enable nagios plugin output mode
> -W <wrta>,<wpl>%               Warning threshold (nagios output mode)
> -C <crta>,<cpl>%               Critical threshold (nagios output mode)
> -h --help                      This message




issues for windows pc:
	windows pc didn't listen 80 default, so you cannot tcpping success(maybe you can try 443), so
	you need make sure which port is opened before tcpping it.