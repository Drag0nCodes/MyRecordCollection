export interface Record {
  id: number;
  cover?: string; // Optional cover art path
  record: string;
  artist: string;
  rating: number;
  tags: string[];
  release: number;
  dateAdded: string;
  tableId?: number;
}

export interface Filters {
  tags: string[];
  rating: { min: number; max: number };
  release: { min: number; max: number };
}