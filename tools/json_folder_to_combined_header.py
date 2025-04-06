# tools/json_folder_to_combined_header_with_array.py

import os
import sys

def to_c_raw_string(var_name, content):
    return f'''// Auto-generated from {var_name.replace('_', '.')}
const char* {var_name} = R"(
{content}
)";
const size_t {var_name}_len = sizeof({var_name}) - 1;
'''

def main():
    if len(sys.argv) != 3:
        print("Usage: python json_folder_to_combined_header_with_array.py <input_folder> <output_file>")
        sys.exit(1)

    input_dir = sys.argv[1]
    output_file = sys.argv[2]

    entries = []
    array_names = []
    array_lengths = []

    for filename in sorted(os.listdir(input_dir)):
        if filename.endswith(".json"):
            json_path = os.path.join(input_dir, filename)
            var_name = filename.replace('.', '_')

            with open(json_path, 'r', encoding='utf-8') as f:
                content = f.read()

            entries.append(to_c_raw_string(var_name, content))
            array_names.append(var_name)
            array_lengths.append(f"{var_name}_len")

    with open(output_file, 'w', encoding='utf-8') as out:
        out.write("// json_blobs.hpp - auto-generated, do not edit\n\n#pragma once\n\n")
        # Include individual JSONs as raw string literals
        for entry in entries:
            out.write(entry)

        # Now, declare the static array of const char* and their lengths
        out.write("\n// Static array of all JSON blobs\n")
        out.write("const char* json_blobs[] = {\n")
        for name in array_names:
            out.write(f"    {name},\n")
        out.write("};\n")

        out.write("\nconst size_t json_blobs_len[] = {\n")
        for length in array_lengths:
            out.write(f"    {length},\n")
        out.write("};\n")

    print(f"Generated combined header with static array: {output_file}")

if __name__ == "__main__":
    main()
