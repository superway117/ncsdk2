# ReadMe

## ncsdk accept test
1. test_async
2. test_sync
3. test_graph
4. test_multigraph
5. test_slow

### how to build

1. remove myd-vsc driver

```
cd hddl-dirvers/myd-vsc
sudo make uninstall
```

2. make related codes

```
make accept=1
```