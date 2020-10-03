#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <hyperon/hyperon.h>

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
std::shared_ptr<T> py_shared_ptr(py::handle pyobj) {
    pyobj.inc_ref();
    return std::shared_ptr<T>(pyobj.cast<T*>(),
            [](T* p) -> void { py::cast(p).dec_ref(); });
}

std::vector<AtomPtr> py_list(py::list atoms) {
    std::vector<AtomPtr> content;
    for (py::handle item : atoms) {
        content.push_back(py_shared_ptr<Atom>(item));
    }
    return content;
}

class PyAtom : public Atom {
public:
    using Atom::Atom;

    bool operator==(Atom const& other) const override {
        PYBIND11_OVERLOAD_PURE_NAME(bool, Atom, "__eq__", operator==, other);
    }

    std::string to_string() const override {
        PYBIND11_OVERLOAD_PURE_NAME(std::string, Atom, "__repr__", to_string,);
    }
};

class PyGroundedAtom : public GroundedAtom {
public:
    using GroundedAtom::GroundedAtom;

    void execute(GroundingSpace const& args, GroundingSpace& result) const override {
        // workaround for a pybind11 issue https://github.com/pybind/pybind11/issues/2033
        // see https://stackoverflow.com/a/59331026/14016260 for explanation
        py::object dummy = py::cast(&result);
        PYBIND11_OVERLOAD(void, GroundedAtom, execute, args, result);
    }

    bool operator==(Atom const& other) const override {
        PYBIND11_OVERLOAD_PURE_NAME(bool, GroundedAtom, "__eq__", operator==, other);
    }

    std::string to_string() const override {
        PYBIND11_OVERLOAD_PURE_NAME(std::string, GroundedAtom, "__repr__", to_string,);
    }
};

struct PyHandleHolder {
    py::handle obj;
    PyHandleHolder(py::handle obj) : obj(obj) { obj.inc_ref(); }
    ~PyHandleHolder() { obj.dec_ref(); }
};

class PyAtomConstr {
public:
    PyAtomConstr(py::object lambda) : lambda(std::make_shared<PyHandleHolder>(lambda)) { }
    AtomPtr operator()(std::string arg) {
        py::object atom = lambda->obj(arg);
        return py_shared_ptr<Atom>(atom);
    }
private:
    std::shared_ptr<PyHandleHolder> lambda;
};

PYBIND11_MODULE(hyperonpy, m) {

    py::class_<SpaceAPI, PySpaceAPI>(m, "SpaceAPI")
        .def(py::init<>())
        .def("add_to", &SpaceAPI::add_to)
        .def("add_from_space", &SpaceAPI::add_from_space)
        .def("add_native", &SpaceAPI::add_native)
        .def("get_type", &SpaceAPI::get_type);

    py::class_<Atom, PyAtom, std::shared_ptr<Atom>> atom(m, "Atom");

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

    m.def("E", [](py::list atoms) -> AtomPtr { return E(py_list(atoms)); });

    py::class_<GroundedAtom, PyGroundedAtom, std::shared_ptr<GroundedAtom>, Atom>(m, "GroundedAtom")
        .def(py::init<>())
        .def("execute", &GroundedAtom::execute)
        .def("__eq__", &GroundedAtom::operator==)
        .def("__repr__", &GroundedAtom::to_string);

    py::class_<GroundingSpace, SpaceAPI>(m, "GroundingSpace")
        .def(py::init<>())
        .def(py::init([](py::list atoms) -> GroundingSpace* {
                        return new GroundingSpace(py_list(atoms));
                    }))
        .def_readonly_static("TYPE", &GroundingSpace::TYPE)
        .def("add_atom", [](GroundingSpace* self, py::object atom) -> void {
                    self->add_atom(py_shared_ptr<Atom>(atom));
                })
        .def("interpret_step", &GroundingSpace::interpret_step)
        .def("match", &GroundingSpace::match)
        .def("get_content", &GroundingSpace::get_content)
        .def("__eq__", &GroundingSpace::operator==)
        .def("__repr__", &GroundingSpace::to_string);
    
    py::class_<TextSpace, SpaceAPI>(m, "TextSpace")
        .def(py::init<>())
        .def_readonly_static("TYPE", &TextSpace::TYPE)
        .def("add_string", &TextSpace::add_string)
        .def("register_token",
                [](TextSpace* self, std::string regex, py::object constr) -> void {
                    self->register_token(std::regex(regex), PyAtomConstr(constr));
                });

    py::class_<Logger> logger(m, "Logger");
    logger.def_static("setLevel", &Logger::setLevel);

    py::enum_<Logger::Level>(logger, "Level")
        .value("ERROR", Logger::Level::ERROR)
        .value("INFO", Logger::Level::INFO)
        .value("DEBUG", Logger::Level::DEBUG)
        .value("TRACE", Logger::Level::TRACE)
        .export_values();
}

