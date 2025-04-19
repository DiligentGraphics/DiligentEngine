# Building Documentation

To build the documentation, install [Doxygen](https://www.doxygen.nl).
Check the required version in the [GitHub Action file](https://github.com/DiligentGraphics/github-action/blob/master/install-doxygen/action.yml).

Then, from the **root** of the repository, run the following commands:

**1. Generate the Markdown pages list**

By default, Doxygen lists all Markdown files in a flat, arbitrary order, which can look messy and isn't
very user-friendly. Use the script below to organize the Markdown files into a tree structure and generate
the `Doc/pages.dox` file:

```bash
python Doc/md_pages.py Doc/pages.dox
```

**2. Build the documentation**

```bash
doxygen Doc/doxygen.cfg
```

Alternatively, you can enable the `DILIGENT_BUILD_DOCS` CMake option to add the `DiligentDocs` target.

The generated documentation will be placed in `build/docs/html`.
Open `build/docs/html/index.html` in your browser to view the documentation.
