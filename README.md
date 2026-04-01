## ISDN3000e-Lab8

This is the lab of the course **ISDN3000e: Programming for Integrative Systems** at HKUST led by Prof. Ziqi Wang. 

The lab requires **Boost.Asio** for TCP. Please install it following the commands below.


### Download Boost (Ubuntu / WSL2)

```bash
sudo apt update
sudo apt install libboost-all-dev
```

### Download Boost (MacOS)

```bash
brew install boost
```

### WSL port
```PowerShell
netsh interface portproxy reset
```
```PowerShell
netsh interface portproxy add v4tov4 `
listenaddress=0.0.0.0 listenport=8888 `
connectaddress=172.29.140.23 connectport=8888
```
```PowerShell
netsh advfirewall firewall add rule name="WSL_8888" dir=in action=allow protocol=TCP localport=8888
```
```PowerShell
netsh interface portproxy show all
```

