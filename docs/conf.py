# Sphinx configuration for the RBMK-SIM documentation and safety case.
# Build (from repo root): .venv/bin/sphinx-build -W -b html docs docs/_build/html

project = "RBMK-SIM"
author = "Gabriel Botan"
copyright = "2026, Gabriel Botan"
release = "0.1.0"

extensions = [
    "myst_parser",
    "sphinxcontrib.mermaid",
]

myst_enable_extensions = [
    "colon_fence",
    "deflist",
    "dollarmath",
    "fieldlist",
]
myst_heading_anchors = 3

templates_path = []
exclude_patterns = ["_build"]

html_theme = "furo"
html_title = "RBMK-SIM — educational reactor simulator"
html_theme_options = {
    "source_repository": "",
    "dark_css_variables": {
        "color-brand-primary": "#7aa2f7",
        "color-brand-content": "#7aa2f7",
    },
}
