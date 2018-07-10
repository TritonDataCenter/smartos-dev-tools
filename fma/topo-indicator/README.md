topo-indicator
--------------
This CLI can be used to get or set the state of any chassis indicator that
is exposed via libtopo.

usage:

# topo-indicator -m <on|off|get> -t <locate|service|ok2rm> <FMRI glob pattern>

example: Get state of chassis identify (locate) indicator:

# topo-indicator -m get -t locate "*chassis=0"

example: Set state of fault (service) indicator for drive bay 10 to ON.

# topo-indicator -m on -t service "*bay=10"
