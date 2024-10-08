## Task Setup
Prerequisite: 2 pairs of key-value existed in the FDB cluster (K1, K2)

```mermaid
sequenceDiagram
    actor T1 as Thread1
    participant fdb as FDBCluster
    actor T2 as Thread2

    Note over fdb: K1, K2 (Version 100)
    
    T1->>fdb: Create Transaction<br>T1
    T2->>fdb: Create Transaction<br>T2

    T1->>fdb: Read K1, Update K2 
    fdb->>T1: V1 (Version 100)

    T2->>fdb: Read K2, Update K1
    fdb->>T2: V2 (Version 100)

    T2->>fdb: Commit Transaction<br>T2
    fdb->>T2: Accepted (Version 101)
    
    Note over fdb: K1 (Version 101)<br>K2 (Version 100)

    T1->>fdb: Commit Transaction<br>T1
    fdb->>T1: Aborted (Conflict on K1)

```

## Comment
T2 will not be aborted because it commits first. However, T1 will be aborted because this transaction read the value that involved in the update operation in the previous committed transaction T2. T1 will only be accepted if snapshot reading for K1 is enabled, because this will not add K1 in the read conflicting set, which will be used by resolvers to check the confliction during the commit phase.

## Execution Log
```
Use cluster file: ../foundationdb/build/fdb.cluster
This program uses client version: 7.3.43,412531b5c97fa84343da94888cc949a4d29e8c29,fdb00b073000000

Network thread started.
Database create successfully.
[INFO] transaction: set_all_kv_pairs committed
[INFO] Try to send txn from auxiliary thread.
[INFO] transaction: read_K2_update_K1 committed
[ERROR] Something wrong in transaction: read_K1_update_K2_should_be_aborted, to rollback... Description: Transaction not committed due to conflict with another transaction
[DEBUG] obtained V1: updated_0000000000000000000000000000000
[DEBUG] obtained V2: value_000000000000000000000000000000001
[INFO] transaction: read_both_for_check committed
[INFO] transaction: cleanup committed
Network thread stopped.
```
