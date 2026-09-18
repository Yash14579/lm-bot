# Gather troop wire order — verified

For v2.200.311, `MarchEventDataType.SetTroopData_T1_T4(uint[])` is verified as **TYPE-MAJOR**.

```text
slots 0..3   Infantry T1,T2,T3,T4
slots 4..7   Ranged   T1,T2,T3,T4
slots 8..11  Cavalry  T1,T2,T3,T4
slots 12..15 Siege    T1,T2,T3,T4
```

The tier-major experiment was rejected and has been reverted.
