## Task setup
Prerequisite: 10k pairs of key-value existed in the FDB cluster

Use getrange to fetch 1k keys each and send multiple(10) transactions to get all pairs.
Try the following streaming mode to compare the performance.
    `FDB_STREAMING_MODE_WANT_ALL`
    `FDB_STREAMING_MODE_ITERATOR` (default)
    `FDB_STREAMING_MODE_EXACT `
    `FDB_STREAMING_MODE_SMALL`
    `FDB_STREAMING_MODE_MEDIUM`
    `FDB_STREAMING_MODE_LARGE`
    `FDB_STREAMING_MODE_SERIAL`

## Reply
I use two ways to issues those 10 transactions, one in synchronous and another in asynchronous.

| Mode                        | synchronous     | asynchronous      | single_range(for reference) |
|:----------------------------|----------------:|------------------:|----------------------------:|
| FDB_STREAMING_MODE_WANT_ALL |   38.761963 ms  |     20.125977 ms  |      20.854980 ms  |
| FDB_STREAMING_MODE_ITERATOR |   25.264160 ms  |     24.807861 ms  |      12.334961 ms  |
| FDB_STREAMING_MODE_EXACT    |   17.376953 ms  |     24.382080 ms  |      11.858887 ms  |
| FDB_STREAMING_MODE_SMALL    |  848.187988 ms  |    785.605957 ms  |     815.907959 ms  |
| FDB_STREAMING_MODE_MEDIUM   |  213.770996 ms  |    215.145020 ms  |     225.569824 ms  |
| FDB_STREAMING_MODE_LARGE    |   83.278076 ms  |     95.407959 ms  |      73.516113 ms  |
| FDB_STREAMING_MODE_SERIAL   |   20.133057 ms  |     20.599854 ms  |      15.108887 ms  |


Most of the case, asynchoronus calls for multiple transactions does not provide better performance
 
## Execution Log
```
Use cluster file: ../foundationdb/build/fdb.cluster
This program uses client version: 7.3.43,412531b5c97fa84343da94888cc949a4d29e8c29,fdb00b073000000

Network thread started.
Database create successfully.
[INFO] transaction: set all 10k pairs committed
[INFO] transaction: sync_get_range_mode_want_all_impl_10k_10txn, batch 1 committed
[INFO] transaction: sync_get_range_mode_want_all_impl_10k_10txn, batch 2 committed
[INFO] transaction: sync_get_range_mode_want_all_impl_10k_10txn, batch 3 committed
[INFO] transaction: sync_get_range_mode_want_all_impl_10k_10txn, batch 4 committed
[INFO] transaction: sync_get_range_mode_want_all_impl_10k_10txn, batch 5 committed
[INFO] transaction: sync_get_range_mode_want_all_impl_10k_10txn, batch 6 committed
[INFO] transaction: sync_get_range_mode_want_all_impl_10k_10txn, batch 7 committed
[INFO] transaction: sync_get_range_mode_want_all_impl_10k_10txn, batch 8 committed
[INFO] transaction: sync_get_range_mode_want_all_impl_10k_10txn, batch 9 committed
[INFO] transaction: sync_get_range_mode_want_all_impl_10k_10txn, batch 10 committed
[INFO] transaction: sync_get_range_mode_iterator_impl_10k_10txn, batch 1 committed
[INFO] transaction: sync_get_range_mode_iterator_impl_10k_10txn, batch 2 committed
[INFO] transaction: sync_get_range_mode_iterator_impl_10k_10txn, batch 3 committed
[INFO] transaction: sync_get_range_mode_iterator_impl_10k_10txn, batch 4 committed
[INFO] transaction: sync_get_range_mode_iterator_impl_10k_10txn, batch 5 committed
[INFO] transaction: sync_get_range_mode_iterator_impl_10k_10txn, batch 6 committed
[INFO] transaction: sync_get_range_mode_iterator_impl_10k_10txn, batch 7 committed
[INFO] transaction: sync_get_range_mode_iterator_impl_10k_10txn, batch 8 committed
[INFO] transaction: sync_get_range_mode_iterator_impl_10k_10txn, batch 9 committed
[INFO] transaction: sync_get_range_mode_iterator_impl_10k_10txn, batch 10 committed
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: sync_get_range_mode_exact_impl_10k_10txn, batch 1 committed
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: sync_get_range_mode_exact_impl_10k_10txn, batch 2 committed
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: sync_get_range_mode_exact_impl_10k_10txn, batch 3 committed
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: sync_get_range_mode_exact_impl_10k_10txn, batch 4 committed
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: sync_get_range_mode_exact_impl_10k_10txn, batch 5 committed
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: sync_get_range_mode_exact_impl_10k_10txn, batch 6 committed
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: sync_get_range_mode_exact_impl_10k_10txn, batch 7 committed
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: sync_get_range_mode_exact_impl_10k_10txn, batch 8 committed
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: sync_get_range_mode_exact_impl_10k_10txn, batch 9 committed
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: sync_get_range_mode_exact_impl_10k_10txn, batch 10 committed
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: sync_get_range_mode_small_impl_10k_10txn, batch 1 committed
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: sync_get_range_mode_small_impl_10k_10txn, batch 2 committed
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: sync_get_range_mode_small_impl_10k_10txn, batch 3 committed
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: sync_get_range_mode_small_impl_10k_10txn, batch 4 committed
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: sync_get_range_mode_small_impl_10k_10txn, batch 5 committed
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: sync_get_range_mode_small_impl_10k_10txn, batch 6 committed
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: sync_get_range_mode_small_impl_10k_10txn, batch 7 committed
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: sync_get_range_mode_small_impl_10k_10txn, batch 8 committed
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: sync_get_range_mode_small_impl_10k_10txn, batch 9 committed
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: sync_get_range_mode_small_impl_10k_10txn, batch 10 committed
[INFO] transaction: sync_get_range_mode_medium_impl_10k_10txn, batch 1 committed
[INFO] transaction: sync_get_range_mode_medium_impl_10k_10txn, batch 2 committed
[INFO] transaction: sync_get_range_mode_medium_impl_10k_10txn, batch 3 committed
[INFO] transaction: sync_get_range_mode_medium_impl_10k_10txn, batch 4 committed
[INFO] transaction: sync_get_range_mode_medium_impl_10k_10txn, batch 5 committed
[INFO] transaction: sync_get_range_mode_medium_impl_10k_10txn, batch 6 committed
[INFO] transaction: sync_get_range_mode_medium_impl_10k_10txn, batch 7 committed
[INFO] transaction: sync_get_range_mode_medium_impl_10k_10txn, batch 8 committed
[INFO] transaction: sync_get_range_mode_medium_impl_10k_10txn, batch 9 committed
[INFO] transaction: sync_get_range_mode_medium_impl_10k_10txn, batch 10 committed
[INFO] transaction: sync_get_range_mode_large_impl_10k_10txn, batch 1 committed
[INFO] transaction: sync_get_range_mode_large_impl_10k_10txn, batch 2 committed
[INFO] transaction: sync_get_range_mode_large_impl_10k_10txn, batch 3 committed
[INFO] transaction: sync_get_range_mode_large_impl_10k_10txn, batch 4 committed
[INFO] transaction: sync_get_range_mode_large_impl_10k_10txn, batch 5 committed
[INFO] transaction: sync_get_range_mode_large_impl_10k_10txn, batch 6 committed
[INFO] transaction: sync_get_range_mode_large_impl_10k_10txn, batch 7 committed
[INFO] transaction: sync_get_range_mode_large_impl_10k_10txn, batch 8 committed
[INFO] transaction: sync_get_range_mode_large_impl_10k_10txn, batch 9 committed
[INFO] transaction: sync_get_range_mode_large_impl_10k_10txn, batch 10 committed
[INFO] transaction: sync_get_range_mode_serial_impl_10k_10txn, batch 1 committed
[INFO] transaction: sync_get_range_mode_serial_impl_10k_10txn, batch 2 committed
[INFO] transaction: sync_get_range_mode_serial_impl_10k_10txn, batch 3 committed
[INFO] transaction: sync_get_range_mode_serial_impl_10k_10txn, batch 4 committed
[INFO] transaction: sync_get_range_mode_serial_impl_10k_10txn, batch 5 committed
[INFO] transaction: sync_get_range_mode_serial_impl_10k_10txn, batch 6 committed
[INFO] transaction: sync_get_range_mode_serial_impl_10k_10txn, batch 7 committed
[INFO] transaction: sync_get_range_mode_serial_impl_10k_10txn, batch 8 committed
[INFO] transaction: sync_get_range_mode_serial_impl_10k_10txn, batch 9 committed
[INFO] transaction: sync_get_range_mode_serial_impl_10k_10txn, batch 10 committed

[Benchmark] Task: sync_get_range_mode_want_all_impl_10k_10txn ------------------
Last transaction status: Success
Item processed: 10000
Transaction processed: 10
Total response time: 38.761963 (ms)
Average response time per 1k item: 3.876196 (ms)
-------------------End of Record-------------------


[Benchmark] Task: sync_get_range_mode_iterator_impl_10k_10txn ------------------
Last transaction status: Success
Item processed: 10000
Transaction processed: 10
Total response time: 25.264160 (ms)
Average response time per 1k item: 2.526416 (ms)
-------------------End of Record-------------------


[Benchmark] Task: sync_get_range_mode_exact_impl_10k_10txn ------------------
Last transaction status: Success
Item processed: 10000
Transaction processed: 10
Total response time: 17.376953 (ms)
Average response time per 1k item: 1.737695 (ms)
-------------------End of Record-------------------


[Benchmark] Task: sync_get_range_mode_small_impl_10k_10txn ------------------
Last transaction status: Success
Item processed: 10000
Transaction processed: 10
Total response time: 848.187988 (ms)
Average response time per 1k item: 84.818799 (ms)
-------------------End of Record-------------------


[Benchmark] Task: sync_get_range_mode_medium_impl_10k_10txn ------------------
Last transaction status: Success
Item processed: 10000
Transaction processed: 10
Total response time: 213.770996 (ms)
Average response time per 1k item: 21.377100 (ms)
-------------------End of Record-------------------


[Benchmark] Task: sync_get_range_mode_large_impl_10k_10txn ------------------
Last transaction status: Success
Item processed: 10000
Transaction processed: 10
Total response time: 83.278076 (ms)
Average response time per 1k item: 8.327808 (ms)
-------------------End of Record-------------------


[Benchmark] Task: sync_get_range_mode_serial_impl_10k_10txn ------------------
Last transaction status: Success
Item processed: 10000
Transaction processed: 10
Total response time: 20.133057 (ms)
Average response time per 1k item: 2.013306 (ms)
-------------------End of Record-------------------

[INFO] transaction: async_get_range_mode_want_all_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_want_all_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_want_all_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_want_all_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_want_all_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_want_all_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_want_all_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_want_all_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_want_all_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_want_all_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_iterator_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_iterator_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_iterator_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_iterator_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_iterator_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_iterator_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_iterator_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_iterator_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_iterator_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_iterator_impl_10k_10txn, batch 10 accepted, to commit
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: async_get_range_mode_exact_impl_10k_10txn, batch 10 accepted, to commit
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: async_get_range_mode_exact_impl_10k_10txn, batch 10 accepted, to commit
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: async_get_range_mode_exact_impl_10k_10txn, batch 10 accepted, to commit
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: async_get_range_mode_exact_impl_10k_10txn, batch 10 accepted, to commit
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: async_get_range_mode_exact_impl_10k_10txn, batch 10 accepted, to commit
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: async_get_range_mode_exact_impl_10k_10txn, batch 10 accepted, to commit
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: async_get_range_mode_exact_impl_10k_10txn, batch 10 accepted, to commit
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: async_get_range_mode_exact_impl_10k_10txn, batch 10 accepted, to commit
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: async_get_range_mode_exact_impl_10k_10txn, batch 10 accepted, to commit
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: async_get_range_mode_exact_impl_10k_10txn, batch 10 accepted, to commit
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: async_get_range_mode_small_impl_10k_10txn, batch 10 accepted, to commit
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: async_get_range_mode_small_impl_10k_10txn, batch 10 accepted, to commit
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: async_get_range_mode_small_impl_10k_10txn, batch 10 accepted, to commit
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: async_get_range_mode_small_impl_10k_10txn, batch 10 accepted, to commit
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: async_get_range_mode_small_impl_10k_10txn, batch 10 accepted, to commit
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: async_get_range_mode_small_impl_10k_10txn, batch 10 accepted, to commit
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: async_get_range_mode_small_impl_10k_10txn, batch 10 accepted, to commit
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: async_get_range_mode_small_impl_10k_10txn, batch 10 accepted, to commit
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: async_get_range_mode_small_impl_10k_10txn, batch 10 accepted, to commit
[WARN] Get 0 pairs even with more indicator from previous run.
[INFO] transaction: async_get_range_mode_small_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_medium_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_medium_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_medium_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_medium_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_medium_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_medium_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_medium_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_medium_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_medium_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_medium_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_large_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_large_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_large_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_large_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_large_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_large_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_large_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_large_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_large_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_large_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_serial_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_serial_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_serial_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_serial_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_serial_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_serial_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_serial_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_serial_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_serial_impl_10k_10txn, batch 10 accepted, to commit
[INFO] transaction: async_get_range_mode_serial_impl_10k_10txn, batch 10 accepted, to commit

[Benchmark] Task: async_get_range_mode_want_all_impl_10k_10txn ------------------
Last transaction status: Success
Item processed: 10000
Transaction processed: 10
Total response time: 20.125977 (ms)
Average response time per 1k item: 2.012598 (ms)
-------------------End of Record-------------------


[Benchmark] Task: async_get_range_mode_iterator_impl_10k_10txn ------------------
Last transaction status: Success
Item processed: 10000
Transaction processed: 10
Total response time: 24.807861 (ms)
Average response time per 1k item: 2.480786 (ms)
-------------------End of Record-------------------


[Benchmark] Task: async_get_range_mode_exact_impl_10k_10txn ------------------
Last transaction status: Success
Item processed: 10000
Transaction processed: 10
Total response time: 24.382080 (ms)
Average response time per 1k item: 2.438208 (ms)
-------------------End of Record-------------------


[Benchmark] Task: async_get_range_mode_small_impl_10k_10txn ------------------
Last transaction status: Success
Item processed: 10000
Transaction processed: 10
Total response time: 785.605957 (ms)
Average response time per 1k item: 78.560596 (ms)
-------------------End of Record-------------------


[Benchmark] Task: async_get_range_mode_medium_impl_10k_10txn ------------------
Last transaction status: Success
Item processed: 10000
Transaction processed: 10
Total response time: 215.145020 (ms)
Average response time per 1k item: 21.514502 (ms)
-------------------End of Record-------------------


[Benchmark] Task: async_get_range_mode_large_impl_10k_10txn ------------------
Last transaction status: Success
Item processed: 10000
Transaction processed: 10
Total response time: 95.407959 (ms)
Average response time per 1k item: 9.540796 (ms)
-------------------End of Record-------------------


[Benchmark] Task: async_get_range_mode_serial_impl_10k_10txn ------------------
Last transaction status: Success
Item processed: 10000
Transaction processed: 10
Total response time: 20.599854 (ms)
Average response time per 1k item: 2.059985 (ms)
-------------------End of Record-------------------

[INFO] transaction: cleanup committed
Network thread stopped.
```
