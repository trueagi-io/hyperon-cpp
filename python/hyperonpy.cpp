#include <initializer_list>
#include <pybind11/pybind11.h>

#include "SpaceAPI.h"
#include "GroundingSpace.h"
#include "TextSpace.h"

namespace py = pybind11;

class PyGroundedExpr : public GroundedExpr {
public:
    using GroundedExpr::GroundedExpr;

    ExprPtr execute(ExprPtr args) const override {
        PYBIND11_OVERLOAD(ExprPtr, GroundedExpr, execute, args);
    }

    bool operator==(Expr const& other) const override {
        PYBIND11_OVERLOAD_PURE(bool, GroundedExpr, operator==, other);
    }

    std::string to_string() const override {
        PYBIND11_OVERLOAD_PURE(std::string, GroundedExpr, to_string,);
    }
};

PYBIND11_MODULE(hyperonpy, m) {

    py::class_<SpaceAPI>(m, "SpaceAPI")
        .def("get_type", &SpaceAPI::get_type)
        .def("add_from_space", &SpaceAPI::add_from_space);

    py::class_<Expr, std::shared_ptr<Expr>> expr(m, "Expr");
    expr.def("get_type", &Expr::get_type)
        .def("__eq__", &Expr::operator==)
        .def("__repr__", &Expr::to_string);

    py::enum_<Expr::Type>(expr, "Type")
        .value("SYMBOL", Expr::Type::SYMBOL)
        .value("GROUNDED", Expr::Type::GROUNDED)
        .value("COMPOSITE", Expr::Type::COMPOSITE)
        .value("VARIABLE", Expr::Type::VARIABLE)
        .export_values();

    m.def("S", &S); 
    m.def("V", &V); 
    m.def("C", (ExprPtr (*)(std::initializer_list<ExprPtr>))&C);
    m.def("C", (ExprPtr (*)(std::vector<ExprPtr>))&C);

    py::class_<GroundedExpr, PyGroundedExpr, std::shared_ptr<GroundedExpr>, Expr>(m, "GroundedAtom")
        .def(py::init<>())
        .def("execute", &GroundedExpr::execute);

    py::class_<GroundingSpace, SpaceAPI>(m, "GroundingSpace")
        .def(py::init<>())
        .def("add_expr", &GroundingSpace::add_expr)
        .def("interpret_step", &GroundingSpace::interpret_step)
        .def("__eq__", &GroundingSpace::operator==)
        .def("__str__", &GroundingSpace::to_string);

    py::class_<TextSpace, SpaceAPI>(m, "TextSpace")
        .def(py::init<>())
        .def("add_string", &TextSpace::add_string)
        .def("register_token", &TextSpace::register_token);
}

