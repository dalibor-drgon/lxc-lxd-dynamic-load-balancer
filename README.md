# lxc-lxd-dynamic-load-balancer
Quick-n-Dirty LXD (lxc) Dynamic load balancer

This is just quick implementation of dynamic load balancer for LXD. Currently it relies on `system()` for command execution, and so the efficiency can be greatly improved by using liblxc api directly. Assumes the load is distributed evenly across multiple CPUs, should work nicely for computers with many cores.
