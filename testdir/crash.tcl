
        sqlite3_crash_enable 1
        sqlite3_test_control_pending_byte 65536
        sqlite3 db test.db -vfs crash
        db eval {
          PRAGMA main.synchronous=FULL;
          BEGIN;
          CREATE TABLE t1(x UNIQUE);
        }
        for {set e 2} {[set e] < (9+2)} {incr e} {
          db eval "CREATE TABLE t[set e] (x)"
        }
        db eval {
          INSERT INTO t1 VALUES( randomblob(170000) );
          COMMIT;
        }
        sqlite3_crash_now
      
