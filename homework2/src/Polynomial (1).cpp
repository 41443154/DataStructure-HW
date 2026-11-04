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

using namespace std;

class Polynomial;  // 前向宣告

class Term {
    friend class Polynomial;
private:
    float coef;  // 係數
    int exp;     // 指數，規定為非負整數
};

namespace detail {

// 不需要額外的 <memory>。
// 這個小工具只負責暫存陣列的自動釋放
//主資料Polynomial::termArray管理。
template <typename T>
class ScopedArray {
    T* data;
public:
    explicit ScopedArray(size_t count) : data(new T[count]) {}
    ~ScopedArray() { delete[] data; }
    T* get() const { return data; }
    T& operator[](size_t index) { return data[index]; }
    T* release() { T* answer = data; data = nullptr; return answer; }
    ScopedArray(const ScopedArray&) = delete;
    ScopedArray& operator=(const ScopedArray&) = delete;
};

// 從 unsigned int 的位元範圍取得 int 最大值
const int MAX_INT_VALUE = static_cast<int>(~0u >> 1);

// <cmath> 定義 最大有限浮點數 HUGE_VALF 與 nextafter 
long double maxFiniteFloat() {
    return static_cast<long double>(nextafter(HUGE_VALF, 0.0f));
}

//逐位解析非負整數，並自行檢查溢位。
//解析非負整數 英文
int parseNonnegativeInt(const string& value, const string& field) {
    if (value.empty()) throw field + "不可為空";
    size_t pos = 0;
    if (value[0] == '+') pos = 1;
    if (pos >= value.size()) throw field + "必須是非負整數";
    int result = 0;
    for (; pos < value.size(); ++pos) {
        if (value[pos] < '0' || value[pos] > '9')
            throw field + "必須是非負整數";
        int digit = value[pos] - '0';
        if (result > (MAX_INT_VALUE - digit) / 10)
            throw field + "超出 int 可表示範圍";
        result = result * 10 + digit;
    }
    return result;
}

float checkedFloat(long double value) {
    if (!isfinite(value) || fabsl(value) > maxFiniteFloat())
        throw string("計算結果超出 float 可表示範圍");
    const float result = static_cast<float>(value);
    if (!isfinite(result) || (result == 0 && value != 0))
        throw string("計算結果太小或超出 float 可表示範圍");
    return result;
}

float parseFiniteFloat(const string& text, const string& field) {
    if (text.empty()) throw field + "不可為空";
    char* end = nullptr;
    const long double value = strtold(text.c_str(), &end);
    if (end == text.c_str() || *end != '\0' || !isfinite(value))
        throw field + "必須是有限數字";
    try {
        return checkedFloat(value);
    } catch (const string&) {
        throw field + "超出 float 可表示範圍";
    }
}

//precision() 即可實現可往返的 float 輸出
// 從 1 到 9 位有效數字挑選能還原原值的最短寫法
string formatFloat(float value) {
    for (int digits = 1; digits <= 9; ++digits) {
        ostringstream output;
        output.precision(digits);
        output << value;
        const string candidate = output.str();
        char* end = nullptr;
        const float parsed = strtof(candidate.c_str(), &end);
        if (end != candidate.c_str() && *end == '\0' && parsed == value)
            return candidate;
    }
    ostringstream output;
    output.precision(9);
    output << value;
    return output.str();
}

string readNonemptyLine(istream& input) {
    string line;
    if (!getline(input >> ws, line)) throw string("輸入已結束");
    return line;
}

}  // namespace detail

class Polynomial {
    friend istream& operator>>(istream& input, Polynomial& poly);
    friend ostream& operator<<(ostream& output, const Polynomial& poly);
private:
    Term* termArray;  // 主儲存：題目指定的動態 Term 陣列
    int capacity;     // 已分配容量
    int terms;        // 非零項數量；指數由大至小，且不重複

    void reserve(int required) {
        if (required <= capacity) return;
        if (required < 0) throw string("陣列容量無效");
        int nextCapacity = capacity ? capacity : 1;
        while (nextCapacity < required) {
            nextCapacity = nextCapacity > detail::MAX_INT_VALUE / 2 ? required : nextCapacity * 2;
        }
        detail::ScopedArray<Term> replacement(nextCapacity);
        for (int i = 0; i < terms; ++i) replacement[i] = termArray[i];
        delete[] termArray;
        termArray = replacement.release();
        capacity = nextCapacity;
    }

    // 僅用在資料已按指數遞減排序、沒有同次項的情況。
    void appendSorted(long double coefficient, int exponent) {
        float value = detail::checkedFloat(coefficient);
        if (value == 0.0f) return;
        if (terms == detail::MAX_INT_VALUE) throw string("項數超出 int 範圍");
        reserve(terms + 1);
        termArray[terms].coef = value;
        termArray[terms].exp = exponent;
        ++terms;
    }

public:
    // Rule of Five：動態陣列必須自行處理複製、移動和解構。
    Polynomial() : termArray(nullptr), capacity(0), terms(0) {}

    Polynomial(const Polynomial& other)
        : termArray(nullptr), capacity(0), terms(0) {
        reserve(other.terms);
        for (int i = 0; i < other.terms; ++i) termArray[i] = other.termArray[i];
        terms = other.terms;
    }

    Polynomial(Polynomial&& other) noexcept
        : termArray(other.termArray), capacity(other.capacity), terms(other.terms) {
        other.termArray = nullptr;
        other.capacity = 0;
        other.terms = 0;
    }

    // copy-and-swap：先複製成功，再換掉舊資料；自我賦值也安全。
    Polynomial& operator=(Polynomial other) noexcept {
        swap(other);
        return *this;
    }

    ~Polynomial() { delete[] termArray; }

    void swap(Polynomial& other) noexcept {
        Term* oldArray = termArray;
        termArray = other.termArray;
        other.termArray = oldArray;
        int oldCapacity = capacity;
        capacity = other.capacity;
        other.capacity = oldCapacity;
        int oldTerms = terms;
        terms = other.terms;
        other.terms = oldTerms;
    }

    // 題目要求的 Add：雙指標掃描，O(m+n)。
    Polynomial Add(Polynomial poly) {
        Polynomial result;
        result.reserve(terms + poly.terms);
        int left = 0, right = 0;
        while (left < terms && right < poly.terms) {
            if (termArray[left].exp > poly.termArray[right].exp) {
                result.appendSorted(termArray[left].coef, termArray[left].exp);
                ++left;
            } else if (termArray[left].exp < poly.termArray[right].exp) {
                result.appendSorted(poly.termArray[right].coef, poly.termArray[right].exp);
                ++right;
            } else {
                const long double sum = static_cast<long double>(termArray[left].coef)
                                      + static_cast<long double>(poly.termArray[right].coef);
                result.appendSorted(sum, termArray[left].exp);
                ++left;
                ++right;
            }
        }
        while (left < terms) {
            result.appendSorted(termArray[left].coef, termArray[left].exp);
            ++left;
        }
        while (right < poly.terms) {
            result.appendSorted(poly.termArray[right].coef, poly.termArray[right].exp);
            ++right;
        }
        return result;
    }

    // 題目要求的 Mult：先產生所有乘積，按指數排序，再合併同次項。
    Polynomial Mult(Polynomial poly) {
        Polynomial result;
        if (terms == 0 || poly.terms == 0) return result;

        struct ProductTerm {
            int exp;
            long double coef;
        };

        const size_t m = static_cast<size_t>(terms);
        const size_t n = static_cast<size_t>(poly.terms);
        if (m > static_cast<size_t>(-1) / n)
            throw string("乘法項數過大");
        const size_t count = m * n;
        if (count > static_cast<size_t>(-1) / sizeof(ProductTerm))
            throw string("乘法暫存空間過大");
        detail::ScopedArray<ProductTerm> products(count);
        size_t written = 0;

        for (int i = 0; i < terms; ++i) {
            for (int j = 0; j < poly.terms; ++j) {
                const long long expSum = static_cast<long long>(termArray[i].exp)
                                       + static_cast<long long>(poly.termArray[j].exp);
                if (expSum > detail::MAX_INT_VALUE) throw string("相乘後的指數超出 int 範圍");
                products[written++] = {
                    static_cast<int>(expSum),
                    static_cast<long double>(termArray[i].coef) * poly.termArray[j].coef
                };
            }
        }

        sort(products.get(), products.get() + count,
             [](const ProductTerm& a, const ProductTerm& b) { return a.exp > b.exp; });

        for (size_t start = 0; start < count;) {
            const int exponent = products[start].exp;
            long double coefficient = 0.0L;
            do {
                coefficient += products[start].coef;
                ++start;
            } while (start < count && products[start].exp == exponent);
            result.appendSorted(coefficient, exponent);
        }
        return result;
    }

    // 題目要求的 Eval：稀疏版 Horner 法，以 long double 降低中間誤差。
    float Eval(float f) {
        if (!isfinite(f)) throw string("代入值必須是有限數字");
        if (terms == 0) return 0.0f;

        long double sum = termArray[0].coef;
        int previousExponent = termArray[0].exp;
        for (int i = 1; i < terms; ++i) {
            const int gap = previousExponent - termArray[i].exp;
            const long double power = pow(static_cast<long double>(f), gap);
            sum = sum * power + termArray[i].coef;
            if (!isfinite(sum)) throw string("代入計算的中間結果過大");
            previousExponent = termArray[i].exp;
        }
        sum *= pow(static_cast<long double>(f), previousExponent);
        return detail::checkedFloat(sum);
    }

    int TermCount() const noexcept { return terms; }
    int Capacity() const noexcept { return capacity; }
    int Degree() const noexcept { return terms == 0 ? -1 : termArray[0].exp; }
    size_t ReservedBytes() const noexcept { return static_cast<size_t>(capacity) * sizeof(Term); }

    // 一行內：項數 係數1 指數1 係數2 指數2 ...，順序可以任意且可重複。
    static Polynomial Parse(const string& line) {
        istringstream source(line);
        string token;
        if (!(source >> token)) throw string("缺少項數");
        const int count = detail::parseNonnegativeInt(token, "項數");
        // 限制單行項數，避免錯誤輸入不合理地耗盡記憶體。
        constexpr int MAX_INPUT_TERMS = 200000;
        if (count > MAX_INPUT_TERMS)
            throw string("單次輸入項數不可超過 200000");

        detail::ScopedArray<Term> entries(count);
        for (int i = 0; i < count; ++i) {
            string coefText, expText;
            if (!(source >> coefText >> expText))
                throw string("資料數量不足：每項需提供係數與指數");
            entries[i].coef = detail::parseFiniteFloat(coefText, "係數");
            entries[i].exp = detail::parseNonnegativeInt(expText, "指數");
        }
        if (source >> token) throw string("存在多餘欄位");
        sort(entries.get(), entries.get() + count,
             [](const Term& a, const Term& b) { return a.exp > b.exp; });

        Polynomial result;
        result.reserve(count);
        for (int i = 0; i < count;) {
            const int exponent = entries[i].exp;
            long double total = 0;
            do {
                total += entries[i].coef;
                ++i;
            } while (i < count && entries[i].exp == exponent);
            result.appendSorted(total, exponent);
        }
        return result;
    }

    string ToString() const {
        if (terms == 0) return "0";
        ostringstream output;
        for (int i = 0; i < terms; ++i) {
            const float coefficient = termArray[i].coef;
            const int exponent = termArray[i].exp;
            if (i == 0) {
                if (coefficient < 0) output << "-";
            } else {
                output << (coefficient < 0 ? " - " : " + ");
            }
            const float magnitude = fabs(coefficient);
            if (exponent == 0 || magnitude != 1.0f)
                output << detail::formatFloat(magnitude);
            if (exponent > 0) {
                output << 'x';
                if (exponent > 1) output << '^' << exponent;
            }
        }
        return output.str();
    }

    // 輸出可再次 Parse 的機器格式，適合存檔；不同於好讀的 << 輸出。
    void Save(ostream& output) const {
        output << terms;
        for (int i = 0; i < terms; ++i) {
            output << ' ' << detail::formatFloat(termArray[i].coef)
                   << ' ' << termArray[i].exp;
        }
        output << '\n';
    }
};

istream& operator>>(istream& input, Polynomial& poly) {
    string line;
    if (!getline(input >> ws, line)) return input;
    try {
        Polynomial candidate = Polynomial::Parse(line);
        poly.swap(candidate);  // 全部檢查成功才替換，不破壞原資料。
    } catch (const string&) {
        input.setstate(ios::failbit);
    }
    return input;
}

ostream& operator<<(ostream& output, const Polynomial& poly) {
    return output << poly.ToString();
}

namespace app {

Polynomial readPolynomialOrThrow(const string& prompt) {
    while (true) {
        cout << prompt << flush;
        const string line = detail::readNonemptyLine(cin);
        try {
            return Polynomial::Parse(line);
        } catch (const string& error) {
            cout << "輸入錯誤：" << error << "，請重新輸入。\n";
        }
    }
}

float readValueOrThrow() {
    while (true) {
        cout << "請輸入 x：" << flush;
        const string line = detail::readNonemptyLine(cin);
        istringstream parser(line);
        string token, extra;
        if ((parser >> token) && !(parser >> extra)) {
            try {
                return detail::parseFiniteFloat(token, "x");
            } catch (const string& error) {
                cout << "輸入錯誤：" << error << "，請重新輸入。\n";
                continue;
            }
        }
        cout << "輸入錯誤：請只輸入一個數字。\n";
    }
}

bool savePair(const string& filename, const Polynomial& a, const Polynomial& b) {
    ofstream file(filename, ios::out | ios::trunc);
    if (!file) return false;
    a.Save(file);
    b.Save(file);
    file.flush();
    return static_cast<bool>(file);
}

void loadPair(const string& filename, Polynomial& a, Polynomial& b) {
    ifstream file(filename);
    if (!file) throw string("無法開啟檔案");
    string first, second, rest;
    if (!getline(file, first) || !getline(file, second))
        throw string("檔案至少需要兩行多項式資料");
    while (getline(file, rest)) {
        if (rest.find_first_not_of(" \t\r\n") != string::npos)
            throw string("檔案包含多餘資料");
    }
    Polynomial nextA = Polynomial::Parse(first);
    Polynomial nextB = Polynomial::Parse(second);
    a.swap(nextA);
    b.swap(nextB);
}

void printStats(const char* name, const Polynomial& p) {
    cout << name << ": " << p << "\n"
         << "  非零項數：" << p.TermCount()
         << "，最高次方：" << (p.Degree() < 0 ? string("無（零多項式）") : to_string(p.Degree()))
         << "，容量：" << p.Capacity()
         << "，Term 陣列配置：約 " << p.ReservedBytes() << " bytes\n";
}

bool runSelfTests(ostream& output) {
    int passed = 0, total = 0;
    auto check = [&](const string& label, bool correct) {
        ++total;
        if (correct) ++passed;
        output << (correct ? "[PASS] " : "[FAIL] ") << label << '\n';
    };
    auto parse = [](const string& s) { return Polynomial::Parse(s); };
    auto nearlyEqual = [](float actual, long double expected) {
        return fabsl(static_cast<long double>(actual) - expected)
             <= 0.0001L * max(1.0L, fabsl(expected));
    };

    Polynomial a = parse("3 3 2 2 1 1 0");
    Polynomial b = parse("2 1 1 -1 0");
    check("基本輸入與輸出", a.ToString() == "3x^2 + 2x + 1" && b.ToString() == "x - 1");
    check("零多項式", parse("0").ToString() == "0");
    check("重複指數與零項消去", parse("4 5 2 -5 2 2 0 -2 0").ToString() == "0");
    check("亂序輸入自動排序", parse("3 1 0 2 1 3 4").ToString() == "3x^4 + 2x + 1");
    check("係數 1 / -1 與符號顯示", parse("3 1 3 -1 1 -1 0").ToString() == "x^3 - x - 1");
    check("加法", a.Add(b).ToString() == "3x^2 + 3x");
    check("乘法", a.Mult(b).ToString() == "3x^3 - x^2 - x - 1");
    check("A(2) = 17", nearlyEqual(a.Eval(2), 17));
    check("負數代入與奇次方", nearlyEqual(parse("1 2 3").Eval(-2), -16));
    check("零多項式加乘", parse("0").Add(a).ToString() == a.ToString()
          && parse("0").Mult(a).ToString() == "0");

    Polynomial copy = a;
    a = parse("1 9 0");
    check("深複製不共用記憶體", copy.ToString() == "3x^2 + 2x + 1");
    copy = copy;
    check("自我賦值", copy.ToString() == "3x^2 + 2x + 1");
    Polynomial moved = static_cast<Polynomial&&>(a);
    check("移動建構", moved.ToString() == "9" && a.ToString() == "0");

    Polynomial preserved = parse("1 7 0");
    istringstream badInput("1 8 -1\n");
    badInput >> preserved;
    check("不合法輸入不破壞原物件", !badInput && preserved.ToString() == "7");
    bool extraRejected = false, nanRejected = false, expOverflow = false, coefOverflow = false;
    try { (void)parse("0 1"); } catch (const string&) { extraRejected = true; }
    try { (void)parse("1 nan 0"); } catch (const string&) { nanRejected = true; }
    try { (void)parse("1 1 2147483647").Mult(parse("1 1 1")); }
    catch (const string&) { expOverflow = true; }
    try { (void)parse("1 3e38 0").Mult(parse("1 3 0")); }
    catch (const string&) { coefOverflow = true; }
    check("拒絕多餘輸入", extraRejected);
    check("拒絕 NaN", nanRejected);
    check("乘法指數溢位防護", expOverflow);
    check("乘法係數溢位防護", coefOverflow);

    ostringstream saved;
    copy.Save(saved);
    Polynomial loaded = parse(saved.str());
    check("儲存與讀取資料可往返", loaded.ToString() == copy.ToString());

    // 固定亂數種子，與獨立的直接代入值交叉檢查 100 組案例。
    mt19937 generator(20261104);
    uniform_int_distribution<int> coefficient(-2, 2);
    uniform_int_distribution<int> exponent(0, 6);
    uniform_int_distribution<int> xValue(-2, 2);
    bool allRandomCorrect = true;
    for (int trial = 0; trial < 100; ++trial) {
        ostringstream textA, textB;
        textA << 5; textB << 5;
        long double expectedA = 0, expectedB = 0;
        const int x = xValue(generator);
        for (int j = 0; j < 5; ++j) {
            int c = coefficient(generator), e = exponent(generator);
            textA << ' ' << c << ' ' << e;
            expectedA += c * pow(static_cast<long double>(x), e);
        }
        for (int j = 0; j < 5; ++j) {
            int c = coefficient(generator), e = exponent(generator);
            textB << ' ' << c << ' ' << e;
            expectedB += c * pow(static_cast<long double>(x), e);
        }
        Polynomial p = parse(textA.str()), q = parse(textB.str());
        if (!nearlyEqual(p.Eval(static_cast<float>(x)), expectedA)
            || !nearlyEqual(q.Eval(static_cast<float>(x)), expectedB)
            || !nearlyEqual(p.Add(q).Eval(static_cast<float>(x)), expectedA + expectedB)
            || !nearlyEqual(p.Mult(q).Eval(static_cast<float>(x)), expectedA * expectedB)) {
            allRandomCorrect = false;
            break;
        }
    }
    check("100 組固定種子隨機交叉驗證", allRandomCorrect);
    output << "總結果：" << passed << '/' << total << " 通過\n";
    return passed == total;
}

// 只能使用老師列出的標頭，不使用 <chrono>；改以大量資料壓力測試驗證結果。
void runStressTest(ostream& output) {
    output << "項數n,加法非零項數,乘法非零項數,運算驗證\n";
    const int sizes[] = {100, 200, 400};
    for (int k = 0; k < 3; ++k) {
        const int n = sizes[k];
        ostringstream source;
        source << n;
        for (int exp = n - 1; exp >= 0; --exp) source << " 1 " << exp;
        Polynomial a = Polynomial::Parse(source.str());
        Polynomial b = a;
        Polynomial sum = a.Add(b);
        Polynomial product = a.Mult(b);
        const bool correct = sum.TermCount() == n && product.TermCount() == 2 * n - 1
            && sum.Eval(1.0f) == 2 * n && product.Eval(1.0f) == n * n;
        output << n << ',' << sum.TermCount() << ',' << product.TermCount()
               << ',' << (correct ? "PASS" : "FAIL") << '\n';
        if (!correct) throw string("大量資料測試未通過");
    }
}

int batchMode() {
    try {
        const Polynomial a = Polynomial::Parse(detail::readNonemptyLine(cin));
        const Polynomial b = Polynomial::Parse(detail::readNonemptyLine(cin));
        istringstream xLine(detail::readNonemptyLine(cin));
        string token, extra;
        if (!(xLine >> token) || (xLine >> extra))
            throw string("x 必須是一個數字");
        const float x = detail::parseFiniteFloat(token, "x");
        // 指定核心方法是非 const；使用副本保留題目原簽名。
        Polynomial left = a, right = b;
        const Polynomial sum = left.Add(right);
        const Polynomial product = left.Mult(right);
        const float resultA = left.Eval(x), resultB = right.Eval(x);
        cout << "A = " << a << '\n'
             << "B = " << b << '\n'
             << "A + B = " << sum << '\n'
             << "A * B = " << product << '\n'
             << "A(" << detail::formatFloat(x) << ") = " << detail::formatFloat(resultA) << '\n'
             << "B(" << detail::formatFloat(x) << ") = " << detail::formatFloat(resultB) << '\n';
        return 0;
    } catch (const string& error) {
        cerr << "輸入或運算錯誤：" << error << '\n';
        return 1;
    }
}

int interactiveMode() {
    cout << "=== Homework 2 | Polynomial 多項式運算系統 ===\n"
         << "輸入格式（每個多項式各一行）：項數 係數1 指數1 係數2 指數2 ...\n"
         << "例如：3 3 2 2 1 1 0，代表 3x^2 + 2x + 1。\n"
         << "允許亂序及同次項；指數限非負整數，係數可輸入有限小數。\n\n";
    Polynomial a, b;
    try {
        a = readPolynomialOrThrow("請輸入 A：");
        b = readPolynomialOrThrow("請輸入 B：");
    } catch (const string&) {
        cout << "輸入結束。\n";
        return 0;
    }

    while (true) {
        cout << "\n========== 操作選單 ==========\n"
             << "1. 顯示 A、B\n"
             << "2. A + B\n"
             << "3. A * B\n"
             << "4. 計算 A(x)\n"
             << "5. 計算 B(x)\n"
             << "6. 重新輸入 A\n"
             << "7. 重新輸入 B\n"
             << "8. 儲存 A、B 到檔案\n"
             << "9. 從檔案載入 A、B\n"
             << "10. 顯示項數、次數與容量\n"
             << "11. 執行內建正確性測試\n"
             << "12. 執行大量資料壓力測試\n"
             << "0. 結束\n"
             << "請選擇：" << flush;
        string choiceText;
        if (!getline(cin, choiceText)) break;
        istringstream choiceInput(choiceText);
        string token, extra;
        if (!(choiceInput >> token) || (choiceInput >> extra)) {
            cout << "選單輸入無效，請重新選擇。\n";
            continue;
        }
        int choice = -1;
        try { choice = detail::parseNonnegativeInt(token, "選單選項"); }
        catch (const string&) { cout << "選單輸入無效，請重新選擇。\n"; continue; }
        if (choice == 0) break;

        try {
            switch (choice) {
                case 1:
                    cout << "A = " << a << "\nB = " << b << '\n';
                    break;
                case 2:
                    cout << "A + B = " << a.Add(b) << '\n';
                    break;
                case 3:
                    cout << "A * B = " << a.Mult(b) << '\n';
                    break;
                case 4: {
                    const float x = readValueOrThrow();
                    cout << "A(" << detail::formatFloat(x) << ") = "
                         << detail::formatFloat(a.Eval(x)) << '\n';
                    break;
                }
                case 5: {
                    const float x = readValueOrThrow();
                    cout << "B(" << detail::formatFloat(x) << ") = "
                         << detail::formatFloat(b.Eval(x)) << '\n';
                    break;
                }
                case 6:
                    a = readPolynomialOrThrow("請重新輸入 A：");
                    cout << "A 已更新。\n";
                    break;
                case 7:
                    b = readPolynomialOrThrow("請重新輸入 B：");
                    cout << "B 已更新。\n";
                    break;
                case 8: {
                    cout << "儲存檔案名稱：" << flush;
                    const string filename = detail::readNonemptyLine(cin);
                    cout << (savePair(filename, a, b) ? "儲存成功。\n" : "儲存失敗。\n");
                    break;
                }
                case 9: {
                    cout << "讀取檔案名稱：" << flush;
                    const string filename = detail::readNonemptyLine(cin);
                    loadPair(filename, a, b);
                    cout << "讀取成功。\n";
                    break;
                }
                case 10:
                    printStats("A", a);
                    printStats("B", b);
                    break;
                case 11:
                    runSelfTests(cout);
                    break;
                case 12:
                    runStressTest(cout);
                    break;
                default:
                    cout << "沒有這個選項，請重新選擇。\n";
                    break;
            }
        } catch (const string& error) {
            if (!cin) { cout << "輸入結束。\n"; break; }
            cout << "操作失敗：" << error << '\n';
        }
    }
    cout << "程式結束。\n";
    return 0;
}

}  // namespace app

int main(int argc, char* argv[]) {
    if (argc == 1) return app::interactiveMode();
    if (argc == 2) {
        const string mode = argv[1];
        if (mode == "--batch") return app::batchMode();
        if (mode == "--self-test") return app::runSelfTests(cout) ? 0 : 1;
        if (mode == "--stress-test") { app::runStressTest(cout); return 0; }
    }
    cerr << "使用方式：Polynomial [--batch | --self-test | --stress-test]\n";
    return 1;
}
