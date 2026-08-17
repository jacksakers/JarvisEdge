from contextlib import contextmanager

from sqlmodel import Session, SQLModel, create_engine

from app.config import get_db_path

_engine = None


def get_engine():
    global _engine
    if _engine is None:
        _engine = create_engine(
            f"sqlite:///{get_db_path()}",
            connect_args={"check_same_thread": False},
        )
    return _engine


def init_db() -> None:
    import app.models  # noqa: F401 — register models on SQLModel.metadata
    SQLModel.metadata.create_all(get_engine())


@contextmanager
def session_scope():
    """Provide a transactional session; commits on clean exit."""
    with Session(get_engine()) as session:
        yield session
        session.commit()
