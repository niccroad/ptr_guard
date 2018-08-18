# ptr_guard
A proposal for a C++ standard library template which enforces a pointer be checked.

## Usage
To use the ptr_guard in your code add this project to your include path and include ptr_guard.h.
This adds the class template std::experimental::ptr_guard. Usage of this is to create the template
with a pointer or smart pointer parameter which then restricts access unless that pointer is valid
(non-null).

```cpp
#include <ptr_guard.h>

// This function will return "Red" when called with an Apple, and "Green" when
// invoked with a nullptr (it also modifies the Apple parameter supplied).
std::string function(std::experimental::ptr_guard<Apple*> anApple) {
    anApple.call(
        [](const Apple& apple) {
            apple.setColor("Red");
        });
    return anApple.call_or(
        [](const Apple& apple) -> std::string {
            return apple.getColor();
        }, "Green");
}
```

A more complete description is provided in the C++ standard proposal in this repo.

## Tests

A suite of tests is in the repo. These should compile into a test executable as long
as Catch2 is provided on the include path. Invoking the test executable with the
command line parameter --list-test-names-only prints a good part of the wording in the
proposal.

## Standardisation Proposal

I think this class template has the potential to be quite useful and so am presently
investigating the interest and effort involved in that. Contributions of improvements
to the proposal wording along with the implementation would be appreciated.
