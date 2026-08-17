import sqlite3
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
    _run_migrations()


def _run_migrations() -> None:
    """Add any model columns missing from existing tables — create_all() only creates new tables."""
    conn = sqlite3.connect(get_db_path())
    try:
        cursor = conn.cursor()
        for table in SQLModel.metadata.tables.values():
            existing_cols = {row[1] for row in cursor.execute(f"PRAGMA table_info({table.name})")}
            if not existing_cols:
                continue  # table doesn't exist yet — create_all() will have made it fresh, nothing to add
            for column in table.columns:
                if column.name not in existing_cols:
                    col_type = column.type.compile(dialect=conn_dialect())
                    cursor.execute(f"ALTER TABLE {table.name} ADD COLUMN {column.name} {col_type}")
        conn.commit()
    finally:
        conn.close()


def conn_dialect():
    from sqlalchemy.dialects import sqlite
    return sqlite.dialect()


@contextmanager
def session_scope():
    """Provide a transactional session; commits on clean exit."""
    with Session(get_engine()) as session:
        yield session
        session.commit()
