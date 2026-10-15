#pragma once

#include <QString>
#include <QStringView>

#include <optional>

namespace island {

// Arithmetic for the launcher. A small recursive-descent parser: never
// evaluates code, only these:
//   + - * / % (modulo) ^ (power, right-assoc), unary -, parentheses,
//   ×, ÷, implicit multiplication (2pi, 3(4+1)),
//   functions sqrt sin cos tan asin acos atan ln log exp abs round floor ceil,
//   constants pi, e. '.' or ',' as decimal separator.
std::optional<double> evaluate(QStringView expression, QString* error = nullptr);

// Whether a query is worth evaluating as math: has a digit and an operator or
// function, and nothing but math characters.
bool looksLikeMath(QStringView text);

// 12 significant digits, no trailing zeros, no exponent for ordinary sizes.
QString formatNumber(double value);

} // namespace island
