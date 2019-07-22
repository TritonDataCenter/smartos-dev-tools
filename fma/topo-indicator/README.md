topo-indicator
--------------
This CLI can be used to get or set the state of any chassis indicator that
is exposed via libtopo.

## Usage

```
# topo-indicator -m <on|off|get> -t <locate|service|ok2rm> <FMRI glob pattern>
```

<b>example:</b> Get state of chassis identify (locate) indicator:

```
# topo-indicator -m get -t locate "*chassis=0"
```

<b>example:</b> Set state of fault (service) indicator for drive bay 10 to ON.

```
# topo-indicator -m on -t service "*ses/enclosure=0/bay=10"
```
