# Experiment6_MainResult_R04

## 编译

```powershell
g++ -std=c++17 -O2 -fopenmp -I..\common main.cpp -o run_exp.exe
```

如果 OpenMP 不可用：

```powershell
g++ -std=c++17 -O2 -I..\common main.cpp -o run_exp.exe
```

## 运行

```powershell
.\run_exp.exe 20000 results.csv 20260428
```
