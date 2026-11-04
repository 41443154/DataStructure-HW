#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

class Polynomial; // 前宣告

class Term {
    friend class Polynomial;
    friend ostream& operator<<(ostream& output, const Polynomial& a);
private:
    float coef; // 係數
    int exp;    // 指數
};

class Polynomial {
    friend istream& operator>>(istream& input, Polynomial& a);
    friend ostream& operator<<(ostream& output, const Polynomial& a);
private:
    Term* termArray; // 非零項的動態陣列
    int capacity;    // 已配置的容量
    int terms;       // 非零項數量

// 計算擴充後的新容量
// 建立新的動態陣列
// 將原本的有效資料複製到新陣列
// 釋放舊陣列的記憶體
// 將指標指向新陣列
// 更新陣列容量
    void row() { // 容量不足時擴充為兩倍
        int newCapacity = capacity * 2;
        Term* temp = new Term[newCapacity];
        copy(termArray, termArray + terms, temp);
        delete[] termArray;
        termArray = temp;
        capacity = newCapacity;
    }
// 如果係數為 0，直接跳過不儲存
// 如果陣列已滿，呼叫 row() 擴充容量
// 儲存新項目的係數
// 儲存新項目的指數
// 將有效項目數量加 1
    void AppendTerm(float coef, int exp) { // 已排序的結果直接接在最後
        if (coef == 0.0f) return;
        if (terms == capacity) row();
        termArray[terms].coef = coef;
        termArray[terms].exp = exp;
        ++terms;
    }
// 交換兩個物件的動態陣列指標
// 交換兩個物件的陣列容量
// 交換兩個物件的有效項目數量
    void Swap(Polynomial& other) {
        swap(termArray, other.termArray);
        swap(capacity, other.capacity);
        swap(terms, other.terms);
    }

public:
    Polynomial() : termArray(new Term[1]), capacity(1), terms(0) {}

    Polynomial(const Polynomial& other)
        : termArray(new Term[other.capacity]),
          capacity(other.capacity), terms(other.terms) {
        copy(other.termArray, other.termArray + terms, termArray);
    }

    Polynomial& operator=(Polynomial other) { // 複製後交換，避免共用陣列
        Swap(other);
        return *this;
    }

    ~Polynomial() { delete[] termArray; }

    void NewTerm(const float theCoeff, const int theExp) {
        if (theCoeff == 0.0f) return; // 只儲存非零項

        // 找到適當位置，讓指數維持由大到小
        int pos = 0;
        while (pos < terms && termArray[pos].exp > theExp) ++pos;

        if (pos < terms && termArray[pos].exp == theExp) {
            // 同次項合併；如果相消為 0，就把該項刪掉
            termArray[pos].coef += theCoeff;
            if (termArray[pos].coef == 0.0f) {
                for (int i = pos; i < terms - 1; ++i)
                    termArray[i] = termArray[i + 1];
                --terms;
            }
            return;
        }

        if (terms == capacity) row();//擴充 
        for (int i = terms; i > pos; --i)
            termArray[i] = termArray[i - 1];
        termArray[pos].coef = theCoeff;
        termArray[pos].exp = theExp;
        ++terms;
    }
// 多項式加法：比較兩個多項式的指數，將相同次方的係數相加，不同次方則直接保留，最後回傳相加結果。
    Polynomial Add(Polynomial b) const {
        Polynomial c;
        int aPos = 0, bPos = 0;
        while (aPos < terms && bPos < b.terms) {
            if (termArray[aPos].exp == b.termArray[bPos].exp) {
                float total = termArray[aPos].coef + b.termArray[bPos].coef;
                c.AppendTerm(total, termArray[aPos].exp);
                ++aPos;
                ++bPos;
            } else if (termArray[aPos].exp > b.termArray[bPos].exp) {
                c.AppendTerm(termArray[aPos].coef, termArray[aPos].exp);
                ++aPos;
            } else {
                c.AppendTerm(b.termArray[bPos].coef, b.termArray[bPos].exp);
                ++bPos;
            }
        }
        for (; aPos < terms; ++aPos)
            c.AppendTerm(termArray[aPos].coef, termArray[aPos].exp);
        for (; bPos < b.terms; ++bPos)
            c.AppendTerm(b.termArray[bPos].coef, b.termArray[bPos].exp);
        return c;
    }
// 多項式乘法：使用雙層迴圈讓兩個多項式的每一項互相相乘，係數相乘、指數相加，再合併相同次方。
    Polynomial Mult(Polynomial b) const {
        Polynomial c;
        // 每一項互相相乘，再由 NewTerm 合併相同次方
        for (int aPos = 0; aPos < terms; ++aPos) {
            for (int bPos = 0; bPos < b.terms; ++bPos) {
                float cf = termArray[aPos].coef * b.termArray[bPos].coef;
                int ep = termArray[aPos].exp + b.termArray[bPos].exp;
                c.NewTerm(cf, ep);
            }
        }
        return c;
    }
// 多項式代入：將輸入的數值 f 代入 x，逐項計算係數乘以 f 的指數次方，最後加總並回傳計算結果。
    float Eval(float f) const {
        float result = 0.0f;
        for (int i = 0; i < terms; ++i)
            result += termArray[i].coef * static_cast<float>(pow(f, termArray[i].exp));
        return result;
    }
};
//200000 是指 一個多項式最多允許輸入 200,000 項（20 萬項）
//operator>> 多項式輸入	
// 多載輸入運算子 >>，讀取多項式的項數、係數和指數，
// 檢查輸入是否合法，並透過 NewTerm 整理資料，
// 確認所有項目輸入成功後，才更新原本的多項式。
istream& operator>>(istream& input, Polynomial& a) {
    int numTerms;
    if (!(input >> numTerms) || numTerms < 0 || numTerms > 200000) { 
        input.setstate(ios::failbit);
        return input;
    }

    Polynomial temp; // 所有資料正確，才更新原物件
    for (int i = 0; i < numTerms; ++i) {
        float coef;
        int exp;
        if (!(input >> coef >> exp) || exp < 0 || !isfinite(coef)) {
            input.setstate(ios::failbit);
            return input;
        }
        temp.NewTerm(coef, exp);
    }
    a = temp;
    return input;
}
//operator<< 多項式輸出
// 多載輸出運算子 <<，將多項式按照數學格式顯示，
// 自動處理正負號、係數為 1、指數為 0 或 1 的情況，
// 如果沒有任何非零項，則直接輸出 0。
ostream& operator<<(ostream& output, const Polynomial& a) {
    if (a.terms == 0) return output << "0";

    for (int i = 0; i < a.terms; ++i) {
        float coef = a.termArray[i].coef;
        int exp = a.termArray[i].exp;

        if (i == 0) {
            if (coef < 0) output << "-";
        } else {
            output << (coef < 0 ? " - " : " + ");
        }

        float absCoef = fabs(coef);
        if (exp == 0 || absCoef != 1.0f) output << absCoef;
        if (exp > 0) {
            output << "x";
            if (exp > 1) output << "^" << exp;
        }
    }
    return output;
}
//ParsePolynomial() 解析多項式
// 將使用者輸入的一整行文字轉換成多項式，
// 並檢查項數、係數、指數及是否有多餘資料，
// 如果輸入格式正確就更新多項式，否則回傳 false。
bool ParsePolynomial(const string& line, Polynomial& poly) {
    istringstream input(line);
    Polynomial temp;
    if (!(input >> temp)) return false;
    input >> ws;
    if (!input.eof()) return false; // 同一行不能多輸其他資料
    poly = temp;
    return true;
}
//ReadPolynomial() 讀取多項式
// 提示使用者輸入多項式 A 或 B，
// 呼叫 ParsePolynomial 檢查輸入格式，
// 如果輸入錯誤就要求重新輸入，直到成功為止。
bool ReadPolynomial(Polynomial& poly, const string& name) {
    string line;
    while (true) {
        cout << "請輸入多項式 " << name << "（項數 係數 指數 ...）：\n";
        if (!getline(cin, line)) return false;
        if (ParsePolynomial(line, poly)) return true;
        cout << "輸入格式錯誤，請重新輸入（指數須為非負整數）。\n";
    }
}
//ParseFloat() 檢查數值
// 將輸入的文字轉換成 float 數值，
// 檢查是否為有效數字，以及是否包含多餘內容，
// 正確則儲存數值並回傳 true，錯誤則回傳 false。
bool ParseFloat(const string& line, float& value) {
    istringstream input(line);
    float temp;
    if (!(input >> temp) || !isfinite(temp)) return false;
    input >> ws;
    if (!input.eof()) return false;
    value = temp;
    return true;
}
// 提示使用者輸入要代入多項式的 x 值，
// 呼叫 ParseFloat 檢查輸入是否有效，
// 若輸入錯誤則要求重新輸入
bool ReadFloat(float& value) {
    string line;
    while (true) {
        cout << "請輸入 x 的值：";
        if (!getline(cin, line)) return false;
        if (ParseFloat(line, value)) return true;
        cout << "請輸入有效的數字。\n";
    }
}
//互動模式  英文 is InteractiveMode
int InteractiveMode() {
    Polynomial a, b;
    cout << "=== Homework 2｜多項式運算系統 ===\n";
    cout << "輸入格式：項數 係數1 指數1 係數2 指數2 ...\n";
    cout << "範例：3 3 2 2 1 1 0，代表 3x^2 + 2x + 1。\n\n";

    if (!ReadPolynomial(a, "A") || !ReadPolynomial(b, "B")) return 0;

    string line;
    while (true) {
        cout << "\n========== 操作選單 ==========\n"
             << "1. 顯示 A、B 多項式\n"
             << "2. 多項式加法 A + B\n"
             << "3. 多項式乘法 A * B\n"
             << "4. 代入數值計算 A(x)\n"
             << "5. 代入數值計算 B(x)\n"
             << "6. 重新輸入多項式 A\n"
             << "7. 重新輸入多項式 B\n"
             << "0. 結束程式\n"
             << "請選擇";
        if (!getline(cin, line)) break;

        istringstream input(line);
        int choice;
        if (!(input >> choice)) {
            cout << "選項錯誤，請輸入 0～7。\n";
            continue;
        }
        input >> ws;
        if (!input.eof() || choice < 0 || choice > 7) {
            cout << "選項錯誤，請輸入 0～7。\n";
            continue;
        }

        if (choice == 0) {
            cout << "程式結束。\n";
            break;
        }
        if (choice == 1) {
            cout << "A(x) = " << a << "\nB(x) = " << b << '\n';
        } else if (choice == 2) {
            cout << "A(x) + B(x) = " << a.Add(b) << '\n';
        } else if (choice == 3) {
            cout << "A(x) * B(x) = " << a.Mult(b) << '\n';
        } else if (choice == 4 || choice == 5) {
            float x;
            if (!ReadFloat(x)) break;
            if (choice == 4)
                cout << "A(" << x << ") = " << a.Eval(x) << '\n';
            else
                cout << "B(" << x << ") = " << b.Eval(x) << '\n';
        } else if (choice == 6) {
            if (!ReadPolynomial(a, "A")) break;
        } else if (choice == 7) {
            if (!ReadPolynomial(b, "B")) break;
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {
return InteractiveMode();
}
