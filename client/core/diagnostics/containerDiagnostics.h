#ifndef CONTAINERDIAGNOSTICS_H
#define CONTAINERDIAGNOSTICS_H

namespace caelispect
{
    struct ContainerDiagnostics
    {
        bool available = false;
        bool portReachable = false;

        virtual ~ContainerDiagnostics() = default;
    };

} // namespace caelispect

#endif // CONTAINERDIAGNOSTICS_H
