#pragma once

#include <string>
#include <vector>

namespace WinApi
{

    struct Extension
    {
        std::string name;
        std::string value;
    };

    [[nodiscard]]
    bool TryToPickFile(
        std::vector<Extension> const & extensions,
        std::string & outFileAddress
    );

    [[nodiscard]]
    bool SaveAs(
        std::vector<Extension> const & extensions,
        std::string & outFilePath
    );
    
}
