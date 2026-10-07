# 41443154

## 四資工二甲 簡嘉亨

## 作業一 之 Problem 1: Ackermann Function

## 解題說明

本題要求分別使用「遞迴」與「非遞迴」兩種方式計算 Ackermann 函數 \(A(m,n)\)。

Ackermann 函數是一個成長非常快速的函數，當 \(m\) 與 \(n\) 稍微增加時，計算量就會變得非常大，因此這題除了練習遞迴，也可以看出遞迴深度與 Stack 的限制。

### Ackermann 函數定義如下：

| \(A(m,n)\) | 條件 |
|---|---|
| \(n+1\) | 當 \(m=0\) |
| \(A(m-1,1)\) | 當 \(n=0\) |
| \(A(m-1,A(m,n-1))\) | 其他情況 |

### 解題策略

1. 遞迴函式：
   - 直接按照題目的三個條件寫成 `if / else if / else`。
   - 每次函式會再次呼叫自己，直到遇到 \(m=0\) 才開始回傳答案。
   - 優點是寫法跟數學定義幾乎完全一樣，很直觀。

2. 非遞迴函式：
   - 不直接呼叫 `Ackermann()` 自己，而是使用一個整數陣列模擬 Stack。
   - `stack[top++] = m` 代表把目前的 \(m\) 放入 Stack。
   - `m = stack[--top]` 代表把 Stack 最上面的 \(m\) 取出來。
   - 使用 `while (top > 0)` 不斷處理，模擬原本遞迴時系統 Call Stack 的行為。
   - 本程式設定 `MAX_SIZE = 1000000`，避免無限制使用記憶體。

## 程式實作

**作業系統 / IDE：**  
Microsoft Visual Studio Code C/C++

**編譯器：**  
g++

### 共用標頭檔 `header.h`

```cpp
#ifndef HEADER_H
#define HEADER_H

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>

#endif
```

### 遞迴程式 `home1_1.cpp`

```cpp
#include "header.h"

using namespace std;

long long Ackermann(int m, long long n)
{
    if (m == 0)
    {
        return n + 1;
    }
    else if (n == 0)
    {
        return Ackermann(m - 1, 1);
    }
    else
    {
        return Ackermann(m - 1, Ackermann(m, n - 1));
    }
}

int main()
{
    int m;
    long long n;

    cout << "Input m and n: ";
    cin >> m >> n;

    if (m < 0 || n < 0)
    {
        cout << "m and n must be non-negative." << endl;
        return 0;
    }

    cout << "A(" << m << ", " << n << ") = "
         << Ackermann(m, n) << endl;

    return 0;
}
```

### 非遞迴程式 `home1_2.cpp`

```cpp
#include "header.h"

using namespace std;

long long Ackermann(int m, long long n)
{
    const int MAX_SIZE = 1000000;

    int* stack = new int[MAX_SIZE];
    int top = 0;

    stack[top++] = m;

    while (top > 0)
    {
        m = stack[--top];

        if (m == 0)
        {
            n = n + 1;
        }
        else if (n == 0)
        {
            n = 1;

            if (top >= MAX_SIZE)
            {
                cout << "Stack overflow." << endl;
                delete[] stack;
                exit(1);
            }

            stack[top++] = m - 1;
        }
        else
        {
            n = n - 1;

            if (top + 2 >= MAX_SIZE)
            {
                cout << "Stack overflow." << endl;
                delete[] stack;
                exit(1);
            }

            stack[top++] = m - 1;
            stack[top++] = m;
        }
    }

    delete[] stack;

    return n;
}

int main()
{
    int m;
    long long n;

    cout << "Input m and n: ";
    cin >> m >> n;

    if (m < 0 || n < 0)
    {
        cout << "m and n must be non-negative." << endl;
        return 0;
    }

    cout << "A(" << m << ", " << n << ") = "
         << Ackermann(m, n) << endl;

    return 0;
}
```

## 效能分析

### 遞迴函式

1. 空間：
   - 每一次遞迴呼叫都會在系統 Call Stack 中留下資料。
   - 當輸入變大時，遞迴深度會快速增加，因此可能發生 Stack Overflow。
   - Ackermann 函數成長速度非常快，所以 \(m \ge 4\) 時通常不適合直接暴力遞迴計算。

2. 時間：
   - \(m=0\) 時只需要一次計算。
   - \(m=1\) 與 \(m=2\) 的成長仍相對容易處理。
   - \(m=3\) 時，函數值已呈指數型成長：
     \[
     A(3,n)=2^{n+3}-3
     \]
   - 當 \(m \ge 4\) 時，計算量會比一般指數成長更快，實際程式很快就會遇到時間或記憶體限制。

### 非遞迴函式

1. 空間：
   - 使用自己建立的 `stack[]` 模擬遞迴。
   - 不會使用同樣深度的函式呼叫 Stack，但仍需要空間保存尚未處理的 \(m\)。
   - 本程式最多配置 1,000,000 個 `int` 作為模擬 Stack。

2. 時間：
   - 非遞迴版本只是把系統遞迴改成自己管理 Stack。
   - 它並沒有改變 Ackermann 函數本身需要處理的大量狀態，因此當輸入變大時仍然會非常慢。
   - 優點主要是可以避免函式遞迴呼叫的額外成本，以及比較容易控制 Stack 的大小。

## 測試與驗證

### 測試案例（遞迴 / 非遞迴）

| 測試案例 | 輸入 \((m,n)\) | 預期輸出 | 遞迴輸出 | 非遞迴輸出 | 結果 |
|:---:|:---:|:---:|:---:|:---:|:---:|
| 測試一 | \(1,1\) | 3 | 3 | 3 | 通過 |
| 測試二 | \(2,1\) | 5 | 5 | 5 | 通過 |
| 測試三 | \(3,2\) | 29 | 29 | 29 | 通過 |
| 測試四 | \(3,4\) | 125 | 125 | 125 | 通過 |
| 測試五 | \(4,0\) | 13 | 13 | 13 | 通過 |

### 編譯與執行指令

```shell
# 遞迴版本
g++ src/home1_1.cpp -std=c++17 -O2 -o home1_1.exe
.\home1_1.exe

# 非遞迴版本
g++ src/home1_2.cpp -std=c++17 -O2 -o home1_2.exe
.\home1_2.exe
```

測試：

```text
Input m and n: 2 1
A(2, 1) = 5

Input m and n: 3 2
A(3, 2) = 29

Input m and n: 3 4
A(3, 4) = 125
```

### 結論

1. Ackermann 函數很適合用來觀察遞迴的特性，因為它的數學定義本身就是遞迴。

2. 遞迴版本的程式碼最簡單，而且跟題目公式幾乎一模一樣，但是輸入變大後會因為大量函式呼叫而消耗很多 Stack。

3. 非遞迴版本使用陣列模擬 Stack，可以把原本由系統管理的遞迴過程改成自己管理。

4. 非遞迴並不代表 Ackermann 函數會突然變快，因為真正的瓶頸是函數本身的成長速度，而不是只有函式呼叫的成本。

## 申論及開發報告

### 選擇遞迴方法的原因

1. 程式邏輯直觀：
   - 當 \(m=0\) 時回傳 \(n+1\)。
   - 當 \(n=0\) 時回傳 \(A(m-1,1)\)。
   - 其他情況回傳 \(A(m-1,A(m,n-1))\)。
   - 三個條件可以直接對應三個程式分支。

2. 容易驗證：
   - 可以直接將程式碼跟數學定義逐行比較。
   - 對於小型輸入，很容易確認程式是否正確。

**優點**
- 程式短而且容易看懂。
- 和 Ackermann 函數的數學定義一致。
- 適合學習遞迴觀念。

**缺點**
- 遞迴深度可能非常大。
- 容易發生 Stack Overflow。
- 當 \(m,n\) 變大時，執行時間會快速增加。

### 選擇非遞迴 Stack 模擬的原因

1. 可以了解遞迴背後其實也是利用 Stack 保存尚未完成的工作。

2. 自己使用陣列與 `top` 管理 Stack，可以清楚看到 push 與 pop 的過程。

3. 可以自行設定最大 Stack 大小，當空間不足時主動停止程式，避免寫出陣列範圍。

**優點**
- 不直接依賴大量函式遞迴呼叫。
- Stack 大小可以自己控制。
- 更容易觀察遞迴轉非遞迴的原理。

**缺點**
- 程式碼比遞迴版本長。
- 需要自己處理 Stack 邊界。
- Ackermann 函數本身計算量仍然非常大。

---

## 作業一 之 Problem 2: Powerset

## 解題說明

本題要求使用遞迴函式產生集合 \(S\) 的所有子集合，也就是 Powerset（冪集）。

如果集合 \(S\) 有 \(n\) 個不同元素，則子集合總數為：

\[
2^n
\]

例如：

```text
S = {a,b,c}
```

Powerset 為：

```text
{(), (a), (b), (c), (a,b), (a,c), (b,c), (a,b,c)}
```

### 解題策略

本程式不是用二進位遮罩，而是使用「依子集合大小產生組合」的遞迴方式。

1. 外層從 `size = 0` 到 `size = n`：
   - 先產生大小為 0 的子集合。
   - 再產生大小為 1 的子集合。
   - 接著大小為 2、3，一直到 \(n\)。

2. `generateSubset()`：
   - `start` 表示下一次可以從哪一個元素開始選。
   - `need` 表示目前還需要再選幾個元素。
   - `current[]` 用來暫存目前選到的元素。
   - 當 `need == 0` 時，代表目前這組子集合已完成，直接輸出。

3. 這樣可以自然得到題目範例的輸出順序：
   - 空集合
   - 一個元素的子集合
   - 兩個元素的子集合
   - ...
   - 完整集合

## 程式實作

### `home2.cpp`

```cpp
#include "header.h"

using namespace std;

void printSubset(string current[], int size, bool& first)
{
    if (!first)
    {
        cout << ", ";
    }

    first = false;

    cout << "(";

    for (int i = 0; i < size; i++)
    {
        cout << current[i];

        if (i != size - 1)
        {
            cout << ",";
        }
    }

    cout << ")";
}

void generateSubset(
    string S[],
    int n,
    int start,
    int need,
    string current[],
    int currentSize,
    bool& first)
{
    if (need == 0)
    {
        printSubset(current, currentSize, first);
        return;
    }

    for (int i = start; i <= n - need; i++)
    {
        current[currentSize] = S[i];

        generateSubset(
            S,
            n,
            i + 1,
            need - 1,
            current,
            currentSize + 1,
            first
        );
    }
}

void powerset(string S[], int n)
{
    string* current = new string[n];

    bool first = true;

    cout << "{";

    for (int size = 0; size <= n; size++)
    {
        generateSubset(
            S,
            n,
            0,
            size,
            current,
            0,
            first
        );
    }

    cout << "}" << endl;

    delete[] current;
}

int main()
{
    int n;

    cout << "Input number of elements: ";
    cin >> n;

    if (n < 0)
    {
        cout << "Invalid size." << endl;
        return 0;
    }

    string* S = new string[n];

    cout << "Input elements: ";

    for (int i = 0; i < n; i++)
    {
        cin >> S[i];
    }

    cout << "Powerset(S) = ";

    powerset(S, n);

    delete[] S;

    return 0;
}
```

## 效能分析

1. 時間複雜度：

\[
O(n \cdot 2^n)
\]

原因：
- 一共有 \(2^n\) 個子集合。
- 每個子集合在輸出時，最壞情況需要輸出 \(n\) 個元素。
- 因此如果把實際輸出成本也算進去，整體時間複雜度可表示為：

\[
O(n \cdot 2^n)
\]

2. 空間複雜度：

\[
O(n)
\]

原因：
- `S[]` 儲存原本的 \(n\) 個輸入元素。
- `current[]` 最多暫存 \(n\) 個元素。
- 遞迴深度最多也是 \(n\)。
- 程式採用邊產生邊輸出的方式，不會先把全部 \(2^n\) 個子集合存起來。

## 測試與驗證

### 測試案例

| 測試案例 | 輸入大小 \(n\) | 元素 | 預期子集數 | 預期輸出 | 結果 |
|:---:|:---:|:---|:---:|:---|:---:|
| 測試一 | 0 | 無 | 1 | `{()}` | 通過 |
| 測試二 | 1 | `a` | 2 | `{(), (a)}` | 通過 |
| 測試三 | 2 | `a b` | 4 | `{(), (a), (b), (a,b)}` | 通過 |
| 測試四 | 3 | `a b c` | 8 | `{(), (a), (b), (c), (a,b), (a,c), (b,c), (a,b,c)}` | 通過 |

### 編譯與執行指令

```shell
g++ src/home2.cpp -std=c++17 -O2 -o home2.exe
.\home2.exe
```

測試：

```text
Input number of elements: 3
Input elements: a b c
Powerset(S) = {(), (a), (b), (c), (a,b), (a,c), (b,c), (a,b,c)}
```

### 結論

1. Powerset 的子集合數量固定為 \(2^n\)，所以當 \(n\) 增加時，答案數量本身就會快速變多。

2. 本程式使用遞迴產生固定大小的組合，再由大小 0 一直做到大小 \(n\)，因此輸出順序可以跟題目的範例相同。

3. `current[]` 只保存目前正在建立的子集合，完成後直接輸出，不需要把全部子集合先存進記憶體。

4. 當 \(n\) 很大時，真正的限制除了遞迴與計算量以外，還包含輸出大量文字所需要的時間。

## 申論及開發報告

### 選擇遞迴組合方法的原因

1. 輸出順序容易控制：
   - 題目範例是先輸出空集合，再輸出一個元素、兩個元素直到完整集合。
   - 使用 `size = 0` 到 `n` 的方式可以自然達成這個順序。

2. 不需要額外的 STL 容器：
   - 使用動態陣列 `current[]` 就可以儲存目前的子集合。
   - 符合本次作業目前使用的標頭檔內容。

3. 記憶體使用量低：
   - 不需要建立一個很大的二維陣列保存全部答案。
   - 每找到一個子集合就直接輸出。

### 優缺點評估

**優點**
- 輸出順序和題目範例一致。
- 遞迴邏輯清楚。
- 額外空間約為 \(O(n)\)。
- 不需要先保存全部 \(2^n\) 個答案。

**缺點**
- 當 \(n\) 增加時，子集合數量 \(2^n\) 會快速增加。
- 輸出大量子集合本身就需要很多時間。
- 本程式假設輸入元素互不相同，如果輸入重複元素，輸出的子集合內容也可能重複。

---

## 總結

本次作業主要練習兩種重要的遞迴問題。

Problem 1 的 Ackermann Function 讓我練習如何直接依照數學定義撰寫遞迴，也練習如何使用陣列模擬 Stack，把遞迴轉換成非遞迴版本。

Problem 2 的 Powerset 則讓我練習利用遞迴產生組合，並了解當答案本身就有 \(2^n\) 個時，即使程式已經節省記憶體，仍然無法避免大量的計算與輸出時間。

透過這次作業，我更了解遞迴、Call Stack、手動 Stack、動態記憶體以及時間與空間複雜度之間的關係。