# docs

MkDocs documentation site for the HibiscusXR project.

```bash
pip install mkdocs-material
mkdocs serve    # local preview
mkdocs build    # output in site/
```

CI builds the site on every push to main and publishes it under `/docs` on
the project Pages site at https://hibiscusxr-37c6a7.gitlab.io/docs/ (job runs
on the `linux-truenas` runner).
