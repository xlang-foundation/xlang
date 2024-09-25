import os
import re
from datetime import datetime

# Get the current year
current_year = datetime.now().year

# Define the license claim template with placeholders for different comment styles
license_claim_template = {
    'block': f"""/*
 * Copyright (C) {current_year} The XLang™ Foundation
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
""",
    'hash': f"""#
# Copyright (C) {current_year} The XLang™ Foundation
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
""",
    'xml': f"""<!--
  Copyright (C) {current_year} The XLang™ Foundation
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
-->
"""
}

# Define file extensions and their corresponding comment styles
extensions_with_comment_styles = {
    '.c': 'block', '.cpp': 'block', '.cc': 'block', '.cxx': 'block',
    '.h': 'block', '.hpp': 'block', '.hxx': 'block', '.inl': 'block',
    '.java': 'block',
    '.js': 'block', '.jsx': 'block', '.ts': 'block', '.tsx': 'block',
    '.py': 'hash', '.pyx': 'hash', '.pxd': 'hash',
    '.x': 'hash',
    '.yaml': 'hash', '.yml': 'hash',
    '.json': 'hash',  # JSON files typically use '//' but we're using '#' for consistency
    '.xml': 'xml', '.html': 'xml', '.htm': 'xml',
    '.cmake': 'hash',  # CMake files
    '.cs': 'block'  # C# files
}

# Add specific filenames like CMakeLists.txt
specific_files = {
    'CMakeLists.txt': 'hash'
}

# Define files and directories to be excluded
exclude_list = [
    '\\ThirdParty',
    '\\Http\\depends\\windows', 
    '\\Http\\httplib.h',
    '\\Image\\turbojpeg\\lib',
    '\\Image\\turbojpeg\\include',
    '\\sqlite\\sqlite',
    '\\Tools\\file_scan_add_lic.py',
]

# Normalize paths in the exclude list to ensure cross-platform compatibility
exclude_list = [os.path.normpath(exclude) for exclude in exclude_list]

def is_excluded(path, exclude_list):
    """Check if a given path should be excluded based on the exclude_list."""
    norm_path = os.path.normpath(path)
    for exclude in exclude_list:
        if exclude in norm_path:
            return True
    return False

def remove_existing_license(content, comment_style):
    """Remove existing license block if found at the top of the file."""
    license_regex_patterns = {
        'block': r"/\*.*?The XLang™ Foundation.*?\*/",
        'hash': r"#.*?The XLang™ Foundation.*?#",
        'xml': r"<!--.*?The XLang™ Foundation.*?-->",
    }
    
    pattern = license_regex_patterns.get(comment_style)
    if pattern:
        content = re.sub(pattern, '', content, flags=re.DOTALL).strip()
    return content

def add_license_to_file(filepath, comment_style):
    """Add license claim to the top of the file based on its comment style."""
    license_claim = license_claim_template[comment_style]
    
    with open(filepath, 'r+', encoding='utf-8') as file:
        content = file.read()
        
        # Remove existing license block if present
        content = remove_existing_license(content, comment_style)
        
        # Add the new license block
        file.seek(0, 0)
        file.write(license_claim + '\n' + content)
        print(f"License added to {filepath}")

def scan_and_add_license(directory):
    """Recursively scan the directory and add license claim to matching files."""
    for root, dirs, files in os.walk(directory):
        # Normalize root path to ensure compatibility
        norm_root = os.path.normpath(root)
        
        # Exclude specified folders
        dirs[:] = [d for d in dirs if not is_excluded(os.path.join(norm_root, d), exclude_list)]
        
        for file in files:
            filepath = os.path.join(norm_root, file)
            
            if is_excluded(filepath, exclude_list):
                continue  # Skip excluded files

            # Determine the comment style based on the file extension or specific file name
            ext = os.path.splitext(file)[1]
            comment_style = extensions_with_comment_styles.get(ext, None)
            if not comment_style:
                comment_style = specific_files.get(file, None)
            
            if comment_style:
                add_license_to_file(filepath, comment_style)

if __name__ == "__main__":
    # Define the directory to be scanned
    directory_to_scan = "D:\\source\\xlang"
    directory_to_scan = os.path.normpath(directory_to_scan)
    
    scan_and_add_license(directory_to_scan)
