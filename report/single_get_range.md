## Task Setup
Prerequisite: 10k pairs of key-value existed in the FDB cluster

Use getrange from `\x00` - `\xff` to fetch all keys in the userspace.
Try the following streaming mode to compare the performance.
- `FDB_STREAMING_MODE_WANT_ALL`
- `FDB_STREAMING_MODE_ITERATOR` (default)
- `FDB_STREAMING_MODE_EXACT `
- `FDB_STREAMING_MODE_SMALL`
- `FDB_STREAMING_MODE_MEDIUM`
- `FDB_STREAMING_MODE_LARGE`
- `FDB_STREAMING_MODE_SERIAL`

## Comment

| Mode                        | Response Time   | Count per call |
|:----------------------------|----------------:| --------------:|
| FDB_STREAMING_MODE_WANT_ALL |   20.854980 ms  |            448 |
| FDB_STREAMING_MODE_ITERATOR |   12.334961 ms  |         Varies |
| FDB_STREAMING_MODE_EXACT    |   11.858887 ms  |           1000 |
| FDB_STREAMING_MODE_SMALL    |  815.907959 ms  |              2 |
| FDB_STREAMING_MODE_MEDIUM   |  225.569824 ms  |              7 |
| FDB_STREAMING_MODE_LARGE    |   73.516113 ms  |             25 |
| FDB_STREAMING_MODE_SERIAL   |   15.108887 ms  |            448 |


The default iterator methods provide the almost the fastest performance, slightly worse than the exact method.
But in the exact mode, the limit of item should be explicitly provided by user, which is not always possible.

The streaming mode will affect the items count (*out_count) returned from each `fdb_future_get_keyvalue_array(FDBFuture* f, FDBKeyValue const** out_kv, int* out_count, fdb_bool_t* out_more)`. Iterator mode will gradually change the limit between each call.

## Execution Log
```
Use cluster file: ../foundationdb/build/fdb.cluster
This program uses client version: 7.3.43,412531b5c97fa84343da94888cc949a4d29e8c29,fdb00b073000000

Network thread started.
Database create successfully.
[INFO] transaction: set all 10k pairs committed
[INFO] transaction: get_range_mode_want_all_impl_10k_1txn, batch 1 committed
[INFO] transaction: get_range_mode_iterator_impl_10k_1txn, batch 1 committed
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: get_range_mode_exact_impl_10k_1txn, batch 1 committed
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: get_range_mode_small_impl_10k_1txn, batch 1 committed
[INFO] transaction: get_range_mode_medium_impl_10k_1txn, batch 1 committed
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: get_range_mode_large_impl_10k_1txn, batch 1 committed
[INFO] transaction: get_range_mode_serial_impl_10k_1txn, batch 1 committed

[Benchmark] Task: get_range_mode_want_all_impl_10k_1txn ------------------
Last transaction status: Success
Item processed: 10000
Transaction processed: 1
Total response time: 20.854980 (ms)
Average response time per 1k item: 2.085498 (ms)
-------------------End of Record-------------------


[Benchmark] Task: get_range_mode_iterator_impl_10k_1txn ------------------
Last transaction status: Success
Item processed: 10000
Transaction processed: 1
Total response time: 12.334961 (ms)
Average response time per 1k item: 1.233496 (ms)
-------------------End of Record-------------------


[Benchmark] Task: get_range_mode_exact_impl_10k_1txn ------------------
Last transaction status: Success
Item processed: 10000
Transaction processed: 1
Total response time: 11.858887 (ms)
Average response time per 1k item: 1.185889 (ms)
-------------------End of Record-------------------


[Benchmark] Task: get_range_mode_small_impl_10k_1txn ------------------
Last transaction status: Success
Item processed: 10000
Transaction processed: 1
Total response time: 815.907959 (ms)
Average response time per 1k item: 81.590796 (ms)
-------------------End of Record-------------------


[Benchmark] Task: get_range_mode_medium_impl_10k_1txn ------------------
Last transaction status: Success
Item processed: 10000
Transaction processed: 1
Total response time: 225.569824 (ms)
Average response time per 1k item: 22.556982 (ms)
-------------------End of Record-------------------


[Benchmark] Task: get_range_mode_large_impl_10k_1txn ------------------
Last transaction status: Success
Item processed: 10000
Transaction processed: 1
Total response time: 73.516113 (ms)
Average response time per 1k item: 7.351611 (ms)
-------------------End of Record-------------------


[Benchmark] Task: get_range_mode_serial_impl_10k_1txn ------------------
Last transaction status: Success
Item processed: 10000
Transaction processed: 1
Total response time: 15.108887 (ms)
Average response time per 1k item: 1.510889 (ms)
-------------------End of Record-------------------

[INFO] transaction: cleanup committed
Network thread stopped.
```
