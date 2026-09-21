# docs

[gitlab.com/neosalsa/docs](https://gitlab.com/neosalsa/docs)

This site. MkDocs + Material, built by GitLab CI on the `linux-truenas`
runner and published to GitLab Pages.

```bash
pip install mkdocs-material
mkdocs serve    # local preview on :8000
mkdocs build    # static output in site/
```

CI: `.gitlab-ci.yml` runs `mkdocs build --strict -d public` on every push to
`main`; the `public/` artifact becomes the Pages deployment.
