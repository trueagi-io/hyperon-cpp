#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "SpaceAPI.h"
#include "GroundingSpace.h"
#include "TextSpace.h"

namespace py = pybind11;

class PySpaceAPI : public SpaceAPI {
public:
    using SpaceAPI::SpaceAPI;

    void add_to(SpaceAPI& graph) const {
        PYBIND11_OVERLOAD_PURE(void, SpaceAPI, add_to, graph);
    }

    void add_from_space(const SpaceAPI& graph) {
        PYBIND11_OVERLOAD_PURE(void, SpaceAPI, add_from_space, graph);
    }

    void add_native(const SpaceAPI* pGraph) {
        PYBIND11_OVERLOAD_PURE(void, SpaceAPI, add_native, pGraph);
    }

    std::string get_type() const {
        PYBIND11_OVERLOAD_PURE(std::string, SpaceAPI, get_type,)
    }
    
};

// Wrap Python object inheriting C++ class into shared_ptr which keeps
// Python reference until shared pointer is released. This is required when
// Python object is returned by virtual method defined in C++ class and
// overriden in Python class. Without inc_ref() Python interpreter releases
// reference just after Python object is returned to caller.
template<typename T>
std::shared_ptr<T> py_shared_ptr(py::object pyobj) {
    pyobj.inc_ref();
    return std::shared_ptr<T>(pyobj.cast<T*>(),
            [](T* p) -> void { py::cast(p).dec_ref(); });
}

class GroundedExprProxy : public GroundedExpr {
public:
    virtual ~GroundedExprProxy() { }

    ExprPtr execute(ExprPtr args) const override {
        py::object py_result = py_execute(py::cast(args));
        return py_shared_ptr<Expr>(py_result);
    }

    virtual py::object py_execute(py::object args) const {
        throw std::runtime_error("Operation is not supported");
    }
};

class PyGroundedExpr : public GroundedExprProxy {
public:
    using GroundedExprProxy::GroundedExprProxy;

    py::object py_execute(py::object args) const {
        PYBIND11_OVERLOAD_NAME(py::object, GroundedExprProxy, "execute", py_execute, args);
    }

    bool operator==(Expr const& other) const override {
        PYBIND11_OVERLOAD_PURE_NAME(bool, GroundedExprProxy, "__eq__", operator==, other);
    }

    std::string to_string() const override {
        PYBIND11_OVERLOAD_PURE_NAME(std::string, GroundedExprProxy, "__repr__", to_string,);
    }
};

class GroundingSpaceProxy : public GroundingSpace {
public:
    void py_add_expr(py::object expr) {
        GroundingSpace::add_expr(py_shared_ptr<Expr>(expr));
    }
};

class TextSpaceProxy : public TextSpace {
public:
    void py_register_token(std::string regex, py::object constr) {
        TextSpace::register_token(std::regex(regex),
                [constr](std::string str) -> ExprPtr {
                    py::object py_result = constr(str);
                    return py_shared_ptr<Expr>(py_result);
                });
    }
};

PYBIND11_MODULE(hyperonpy, m) {

    py::class_<SpaceAPI, PySpaceAPI>(m, "SpaceAPI")
        .def(py::init<>())
        .def("add_to", &SpaceAPI::add_to)
        .def("add_from_space", &SpaceAPI::add_from_space)
        .def("add_native", &SpaceAPI::add_native)
        .def("get_type", &SpaceAPI::get_type);

    py::class_<Expr, std::shared_ptr<Expr>> atom(m, "Atom");

    atom.def("get_type", &Expr::get_type)
        .def("__eq__", &Expr::operator==)
        .def("__repr__", &Expr::to_string);

    py::enum_<Expr::Type>(atom, "Type")
        .value("SYMBOL", Expr::Type::SYMBOL)
        .value("GROUNDED", Expr::Type::GROUNDED)
        .value("COMPOSITE", Expr::Type::COMPOSITE)
        .value("VARIABLE", Expr::Type::VARIABLE)
        .export_values();

    py::class_<SymbolExpr, std::shared_ptr<SymbolExpr>, Expr>(m, "SymbolAtom")
        .def(py::init<std::string>())
        .def("get_symbol", &SymbolExpr::get_symbol);

    m.def("S", &S); 

    py::class_<VariableExpr, std::shared_ptr<VariableExpr>, Expr>(m, "VariableAtom")
        .def(py::init<std::string>())
        .def("get_name", &VariableExpr::get_name);

    m.def("V", &V); 

    // TODO: bingings below are not effective as pybind11 by default copies
    // vectors when converting Python lists to them and vice versa. Better way
    // is adapting Python list interface to std::vector or introduce light
    // container interface instead of std::vector and adapt Python list to it.
    py::class_<CompositeExpr, std::shared_ptr<CompositeExpr>, Expr>(m, "CompositeAtom")
        .def(py::init<std::vector<ExprPtr>>())
        .def("get_children", &CompositeExpr::get_children);

    // FIXME: replace keep_alive by py_shared_ptr
    m.def("C", (ExprPtr (*)(std::vector<ExprPtr>))&C, py::keep_alive<0, 1>());

    py::class_<GroundedExprProxy, PyGroundedExpr, std::shared_ptr<GroundedExprProxy>, Expr>(m, "GroundedAtom")
        .def(py::init<>())
        .def("execute", &GroundedExprProxy::py_execute);

    py::class_<GroundingSpaceProxy, SpaceAPI>(m, "GroundingSpace")
        .def(py::init<>())
        .def_readonly_static("TYPE", &GroundingSpaceProxy::TYPE)
        .def("add_expr", &GroundingSpaceProxy::py_add_expr)
        .def("interpret_step", &GroundingSpaceProxy::interpret_step)
        .def("__eq__", &GroundingSpaceProxy::operator==)
        .def("__repr__", &GroundingSpaceProxy::to_string);

    py::class_<TextSpaceProxy, SpaceAPI>(m, "TextSpace")
        .def(py::init<>())
        .def_readonly_static("TYPE", &TextSpaceProxy::TYPE)
        .def("add_string", &TextSpaceProxy::add_string)
        .def("register_token", &TextSpaceProxy::py_register_token);
}

