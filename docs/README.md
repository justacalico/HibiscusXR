# docs

MkDocs documentation site for the PN2Lineage project.

```bash
pip install mkdocs-material
mkdocs serve    # local preview
mkdocs build    # output in site/
```

CI builds the site on every push to main and publishes it with GitLab Pages
at https://neosalsa.gitlab.io/docs (job runs on the `linux-truenas` runner).
