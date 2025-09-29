import { DataGrid, type GridColDef } from "@mui/x-data-grid";
import { type Record } from "../types";
import placeholderCover from "../assets/missingImg.jpg";

// Clean column definitions with wrapping via cellClassName
const columns: GridColDef[] = [
  {
    field: "cover",
    headerName: "",
    width: 110,
    sortable: false,
    renderCell: (params) => {
      const src =
        (params.row.cover === "" && placeholderCover) || params.row.cover;
      const title = params.row.record ?? "cover";
      return (
        <img
          src={src}
          alt={title}
          style={{
            maxWidth: 100,
            maxHeight: 100,
            objectFit: "cover",
            borderRadius: 6,
          }}
        />
      );
    },
  },
  {
    field: "record",
    headerName: "Record",
    flex: 1.5,
    minWidth: 200,
    hideable: false,
    cellClassName: "wrapCell",
    renderCell: (params) => (
      <div className="wrapText" style={{ width: "100%" }}>
        {params.value}
      </div>
    ),
  },
  {
    field: "artist",
    headerName: "Artist",
    flex: 1.25,
    minWidth: 100,
    hideable: false,
    cellClassName: "wrapCell",
    renderCell: (params) => (
      <div className="wrapText" style={{ width: "100%" }}>
        {params.value}
      </div>
    ),
  },
  {
    field: "rating",
    headerName: "Rating",
    type: "number",
    flex: 0.5,
    minWidth: 90,
    filterable: false,
    align: "left",
    headerAlign: "left",
  },
  {
    field: "tags",
    headerName: "Tags",
    flex: 1.5,
    minWidth: 120,
    sortable: false,
    hideable: false,
    filterable: false,
    valueGetter: (value: string[]) => value.join(", "),
    cellClassName: "wrapCell",
    renderCell: (params) => (
      <div className="wrapText" style={{ width: "100%" }}>
        {params.value}
      </div>
    ),
  },
  {
    field: "release",
    headerName: "Release",
    type: "number",
    flex: 0.5,
    minWidth: 100,
    align: "left",
    headerAlign: "left",
    filterable: false,
    renderCell: (params) => {
      const val = params.value;
      if (typeof val === "number") return val.toString();
      return val;
    },
  },
  {
    field: "dateAdded",
    headerName: "Date Added",
    flex: 1,
    minWidth: 110,
    renderCell: (params) => {
      const val = params.value;
      if (typeof val === "string") return val.slice(0, 10);
      return val;
    },
  },
];

interface RecordTableProps {
  records: Record[];
  selectedId?: number;
  onSelect?: (record: Record | null) => void;
}

export default function RecordTable({
  records,
  selectedId,
  onSelect,
}: RecordTableProps) {
  const handleRowClick = (params: any) => {
    onSelect?.(params.row as Record);
  };

  const getRowClassName = (params: any) =>
    params.id == selectedId ? "selected-row" : "";

  const initialState = {
    sorting: { sortModel: [{ field: "rating", sort: "desc" }] },
  } as any;

  return (
    <DataGrid
      rows={records}
      columns={columns}
      initialState={initialState}
      density="comfortable"
      rowHeight={90}
      getRowId={(row) => row.id}
      onRowClick={handleRowClick}
      getRowClassName={getRowClassName}
      checkboxSelection={false}
      disableRowSelectionOnClick={false}
      hideFooterSelectedRowCount
      sx={{
        border: "none",
        height: "100%",
        "& .MuiDataGrid-cell": {
          display: "flex",
          alignItems: "center",
          py: 1,
          // allow content to shrink so wrapping can occur
          minWidth: 0,
        },
        "& .wrapCell .MuiDataGrid-cellContent": {
          whiteSpace: "normal",
          overflow: "hidden",
          textOverflow: "clip",
          overflowWrap: "anywhere",
          lineHeight: 1.2,
          display: "block",
        },
        // stronger override in case cellContent class changes in version updates
        "& .wrapCell": {
          whiteSpace: "normal !important",
        },
        "& .wrapCell .wrapText": {
          whiteSpace: "normal",
          overflowWrap: "anywhere",
          wordBreak: "break-word",
          lineHeight: 1.2,
          alignSelf: "center",
        },
      }}
    />
  );
}
