import os

def compute_page_id(root_dir, input_filepath):
    """
    Compute a unique page id based on the file's path relative to the project root.
    
    For example, if input_filepath is:
      <root_dir>/Path/To/page.md
    then the returned page id becomes:
      Path_To_page
    """    # Compute the file's absolute path and then its relative path from the root directory.
    abs_input_path = os.path.abspath(input_filepath)
    rel_path = os.path.relpath(abs_input_path, root_dir)
    # Remove the file extension.
    rel_path_no_ext, _ = os.path.splitext(rel_path)
    # Replace path separators with underscores.
    page_id = rel_path_no_ext.replace(os.sep, "_")
    return page_id

def get_project_root():
    """
    Get the project root directory.
    
    The project root is defined as the directory containing the script.
    """
    # Determine the directory of this script.
    script_dir = os.path.dirname(os.path.abspath(__file__))
    # The project root is one level up.
    return os.path.dirname(script_dir)