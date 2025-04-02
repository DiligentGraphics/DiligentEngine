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
