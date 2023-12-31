#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <optional>
#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet);
    ~Cell();

    template <typename VerifyFormula>
    void Set(std::string text, VerifyFormula verify_formula);
    void Set(std::string text) {
        return Set(text, [](const FormulaInterface&) {});
    }
    void Clear();
    // true если значение вычислено или не требует других значений.
    bool HasValue() const;
    // true если кеш был очищен.
    bool ClearCache() const;

    Value GetValue() const override;
    std::string GetText() const override;
    const std::vector<Position>& GetReferencedCells() const override;

    bool IsReferenced() const;

private:
    struct EmptyCell {
    };
    struct TextCell {
        std::string text;
        std::string value;
    };
    struct FormulaCell {
        std::unique_ptr<FormulaInterface> formula;
        mutable std::optional<Cell::Value> value;
    };
    Sheet& sheet_;
    // Используем sheet для вычисления формулы в момент создания ячейки.
    std::variant<EmptyCell, TextCell, FormulaCell> calculated_formula = EmptyCell{};
};

template <typename VerifyFormula>
void Cell::Set(std::string text, VerifyFormula verify_formula) {
    if (text.empty()) {
        calculated_formula = EmptyCell{};
    } else if (text[0] == '\'') {
        std::string value = text.substr(1);
        calculated_formula = TextCell{std::move(text), std::move(value)};
    } else if (text == "=") {
        calculated_formula = TextCell{"=", "="};
    } else if (text[0] == '=') {
        std::unique_ptr<FormulaInterface> formula = ParseFormula(text.substr(1));
        verify_formula(*formula);
        calculated_formula = FormulaCell{std::move(formula), {}};
    } else {
        calculated_formula = TextCell{text, std::move(text)};
    }
}