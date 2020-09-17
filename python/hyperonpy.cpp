#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "SpaceAPI.h"
#include "GroundingSpace.h"
#include "TextSpace.h"

namespace py = pybind11;

class PySpaceAPI : public SpaceAPI {
public:
    using SpaceAPI::SpaceAPI;

    void add_to(SpaceAPI& graph) const override {
        PYBIND11_OVERLOAD_PURE(void, SpaceAPI, add_to, graph);
    }

    void add_from_space(const SpaceAPI& graph) override {
        PYBIND11_OVERLOAD_PURE(void, SpaceAPI, add_from_space, graph);
    }

    void add_native(const SpaceAPI* pGraph) override {
        PYBIND11_OVERLOAD_PURE(void, SpaceAPI, add_native, pGraph);
    }

    std::string get_type() const override {
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

class GroundedAtomProxy : public GroundedAtom {
public:
    virtual ~GroundedAtomProxy() { }

    AtomPtr execute(AtomPtr args) const override {
        py::object py_result = py_execute(py::cast(args));
        return py_shared_ptr<Atom>(py_result);
    }

    virtual py::object py_execute(py::object args) const {
        throw std::runtime_error("Operation is not supported");
    }
};

class PyGroundedAtom : public GroundedAtomProxy {
public:
    using GroundedAtomProxy::GroundedAtomProxy;

    py::object py_execute(py::object args) const override {
        PYBIND11_OVERLOAD_NAME(py::object, GroundedAtomProxy, "execute", py_execute, args);
    }

    bool operator==(Atom const& other) const override {
        PYBIND11_OVERLOAD_PURE_NAME(bool, GroundedAtomProxy, "__eq__", operator==, other);
    }

    std::string to_string() const override {
        PYBIND11_OVERLOAD_PURE_NAME(std::string, GroundedAtomProxy, "__repr__", to_string,);
    }
};

class GroundingSpaceProxy : public GroundingSpace {
public:
    void py_add_atom(py::object atom) {
        GroundingSpace::add_atom(py_shared_ptr<Atom>(atom));
    }
};

class TextSpaceProxy : public TextSpace {
public:
    void py_register_token(std::string regex, py::object constr) {
        TextSpace::register_token(std::regex(regex),
                [constr](std::string str) -> AtomPtr {
                    py::object py_result = constr(str);
                    return py_shared_ptr<Atom>(py_result);
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

    py::class_<Atom, std::shared_ptr<Atom>> atom(m, "Atom");

    atom.def("get_type", &Atom::get_type)
        .def("__eq__", &Atom::operator==)
        .def("__repr__", &Atom::to_string);

    py::enum_<Atom::Type>(atom, "Type")
        .value("SYMBOL", Atom::Type::SYMBOL)
        .value("GROUNDED", Atom::Type::GROUNDED)
        .value("EXPR", Atom::Type::EXPR)
        .value("VARIABLE", Atom::Type::VARIABLE)
        .export_values();

    py::class_<SymbolAtom, std::shared_ptr<SymbolAtom>, Atom>(m, "SymbolAtom")
        .def(py::init<std::string>())
        .def("get_symbol", &SymbolAtom::get_symbol);

    m.def("S", &S); 

    py::class_<VariableAtom, std::shared_ptr<VariableAtom>, Atom>(m, "VariableAtom")
        .def(py::init<std::string>())
        .def("get_name", &VariableAtom::get_name);

    m.def("V", &V); 

    // TODO: bingings below are not effective as pybind11 by default copies
    // vectors when converting Python lists to them and vice versa. Better way
    // is adapting Python list interface to std::vector or introduce light
    // container interface instead of std::vector and adapt Python list to it.
    py::class_<ExprAtom, std::shared_ptr<ExprAtom>, Atom>(m, "ExprAtom")
        .def(py::init<std::vector<AtomPtr>>())
        .def("get_children", &ExprAtom::get_children);

    // FIXME: replace keep_alive by py_shared_ptr
    m.def("E", (AtomPtr (*)(std::vector<AtomPtr>))&E, py::keep_alive<0, 1>());

    py::class_<GroundedAtomProxy, PyGroundedAtom, std::shared_ptr<GroundedAtomProxy>, Atom>(m, "GroundedAtom")
        .def(py::init<>())
        .def("execute", &GroundedAtomProxy::py_execute);

    py::class_<GroundingSpaceProxy, SpaceAPI>(m, "GroundingSpace")
        .def(py::init<>())
        .def_readonly_static("TYPE", &GroundingSpaceProxy::TYPE)
        .def("add_atom", &GroundingSpaceProxy::py_add_atom)
        .def("interpret_step", &GroundingSpaceProxy::interpret_step)
        .def("match", &GroundingSpaceProxy::match)
        .def("__eq__", &GroundingSpaceProxy::operator==)
        .def("__repr__", &GroundingSpaceProxy::to_string);

    py::class_<TextSpaceProxy, SpaceAPI>(m, "TextSpace")
        .def(py::init<>())
        .def_readonly_static("TYPE", &TextSpaceProxy::TYPE)
        .def("add_string", &TextSpaceProxy::add_string)
        .def("register_token", &TextSpaceProxy::py_register_token);
}

